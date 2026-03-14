#include "video/premiereexporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontInfo>
#include <QUrl>
#include <QXmlStreamWriter>
#include <cmath>

extern "C" {
#include <libavformat/avformat.h>
}

namespace {

struct RateInfo { int timebase; bool ntsc; };
struct SourceMediaInfo { int width = 0; int height = 0; int audioChannels = 0; };

RateInfo fpsToRate(double fps)
{
    RateInfo r{60, false};
    if (std::abs(fps - 23.976) < 0.05)     { r = {24, true}; }
    else if (std::abs(fps - 29.97) < 0.05) { r = {30, true}; }
    else if (std::abs(fps - 59.94) < 0.05) { r = {60, true}; }
    else if (std::abs(fps - 47.952) < 0.05){ r = {48, true}; }
    else { r.timebase = static_cast<int>(std::round(fps)); }
    return r;
}

SourceMediaInfo probeSource(const QString &path)
{
    SourceMediaInfo info;
    AVFormatContext *fmt = nullptr;
    if (avformat_open_input(&fmt, path.toUtf8().constData(), nullptr, nullptr) < 0)
        return info;
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        return info;
    }
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        auto *par = fmt->streams[i]->codecpar;
        if (par->codec_type == AVMEDIA_TYPE_VIDEO && info.width == 0) {
            info.width  = par->width;
            info.height = par->height;
        }
        if (par->codec_type == AVMEDIA_TYPE_AUDIO && info.audioChannels == 0) {
            info.audioChannels = par->ch_layout.nb_channels;
        }
    }
    avformat_close_input(&fmt);
    return info;
}

QString toFileUrl(const QString &localPath)
{
    QString fwd = localPath;
    fwd.replace('\\', '/');
    if (!fwd.startsWith('/')) fwd.prepend('/');
    QByteArray utf = fwd.toUtf8();
    QString out;
    for (int i = 0; i < utf.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(utf[i]);
        if (c == '/' || c == ':' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' || c == ' ')
            out.append(QChar(c));
        else
            out.append(QString("%%%1").arg(c, 2, 16, QChar('0')).toUpper());
    }
    return QStringLiteral("file://localhost") + out;
}

// ───────── XML micro-writers ─────────

void writeRate(QXmlStreamWriter &x, const RateInfo &r)
{
    x.writeStartElement("rate");
    x.writeTextElement("timebase", QString::number(r.timebase));
    x.writeTextElement("ntsc", r.ntsc ? "TRUE" : "FALSE");
    x.writeEndElement();
}

void writeTimecode(QXmlStreamWriter &x, const RateInfo &r)
{
    x.writeStartElement("timecode");
    writeRate(x, r);
    x.writeTextElement("string", "00:00:00:00");
    x.writeTextElement("frame", "0");
    x.writeTextElement("displayformat", "NDF");
    x.writeEndElement();
}

void writeLink(QXmlStreamWriter &x, const QString &ref,
               const QString &media, int track, int clip)
{
    x.writeStartElement("link");
    x.writeTextElement("linkclipref", ref);
    x.writeTextElement("mediatype", media);
    x.writeTextElement("trackindex", QString::number(track));
    x.writeTextElement("clipindex", QString::number(clip));
    x.writeEndElement();
}

void writeScaleFilter(QXmlStreamWriter &x, double scalePct)
{
    x.writeStartElement("filter");
    x.writeStartElement("effect");
    x.writeTextElement("name", "Basic Motion");
    x.writeTextElement("effectid", "basic");
    x.writeTextElement("effectcategory", "motion");
    x.writeTextElement("effecttype", "motion");
    x.writeTextElement("mediatype", "video");

    x.writeStartElement("parameter");
    x.writeTextElement("parameterid", "scale");
    x.writeTextElement("name", "Scale");
    x.writeTextElement("valuemin", "0");
    x.writeTextElement("valuemax", "1000");
    x.writeTextElement("value", QString::number(scalePct, 'f', 2));
    x.writeEndElement();

    x.writeEndElement(); // effect
    x.writeEndElement(); // filter
}

void writeOpacityFilter(QXmlStreamWriter &x, double opacityPct)
{
    x.writeStartElement("filter");
    x.writeStartElement("effect");
    x.writeTextElement("name", "Opacity");
    x.writeTextElement("effectid", "opacity");
    x.writeTextElement("effectcategory", "motion");
    x.writeTextElement("effecttype", "motion");
    x.writeTextElement("mediatype", "video");

    x.writeStartElement("parameter");
    x.writeTextElement("parameterid", "opacity");
    x.writeTextElement("name", "Opacity");
    x.writeTextElement("valuemin", "0");
    x.writeTextElement("valuemax", "100");
    x.writeTextElement("value", QString::number(opacityPct, 'f', 1));
    x.writeEndElement();

    x.writeEndElement(); // effect
    x.writeEndElement(); // filter
}

QString fcpText(const QString &s)
{
    QString out = s;
    out.replace('\n', '\r');
    return out;
}

void writeTextParam(QXmlStreamWriter &x, const QString &pid,
                    const QString &name, const QString &val)
{
    x.writeStartElement("parameter");
    x.writeTextElement("parameterid", pid);
    x.writeTextElement("name", name);
    x.writeTextElement("value", val);
    x.writeEndElement();
}

void writeNumParam(QXmlStreamWriter &x, const QString &pid,
                   const QString &name, double val)
{
    x.writeStartElement("parameter");
    x.writeTextElement("parameterid", pid);
    x.writeTextElement("name", name);
    x.writeTextElement("value", QString::number(val, 'f', 1));
    x.writeEndElement();
}

void writeColorParam(QXmlStreamWriter &x, const QString &pid,
                     const QString &name, const QColor &c)
{
    x.writeStartElement("parameter");
    x.writeTextElement("parameterid", pid);
    x.writeTextElement("name", name);
    x.writeStartElement("value");
    x.writeTextElement("alpha", QString::number(c.alpha()));
    x.writeTextElement("red",   QString::number(c.red()));
    x.writeTextElement("green", QString::number(c.green()));
    x.writeTextElement("blue",  QString::number(c.blue()));
    x.writeEndElement();
    x.writeEndElement();
}

double fcpFontSize(double programPtSize, int seqHeight)
{
    return programPtSize * 480.0 / seqHeight;
}

QString resolveFontFamily(const QFont &font)
{
    QFontInfo fi(font);
    return fi.family();
}

void writeTextGeneratorEffect(QXmlStreamWriter &x,
                              const QString &text,
                              const QFont &font,
                              double fontSize, int seqH,
                              const QColor &color,
                              bool outlineEnabled,
                              const QColor &outlineColor,
                              float outlineWidth)
{
    bool useOutline = outlineEnabled && outlineWidth > 0;
    double fcpSize = fcpFontSize(fontSize, seqH);
    double fcpOutline = fcpFontSize(outlineWidth, seqH);
    QString family = resolveFontFamily(font);

    x.writeStartElement("effect");
    if (useOutline) {
        x.writeTextElement("name", "Outline Text");
        x.writeTextElement("effectid", "Outline Text");
    } else {
        x.writeTextElement("name", "Text");
        x.writeTextElement("effectid", "Text");
    }
    x.writeTextElement("effectcategory", "Text");
    x.writeTextElement("effecttype", "generator");
    x.writeTextElement("mediatype", "video");

    writeTextParam(x, "str", "Text", fcpText(text));
    writeTextParam(x, "font", "Font", family);
    writeNumParam(x, "size", "Size", fcpSize);

    int style = 1;
    if (font.bold() && font.italic()) style = 4;
    else if (font.bold()) style = 2;
    else if (font.italic()) style = 3;
    writeNumParam(x, "style", "Style", style);

    writeNumParam(x, "alignment", "Alignment", 2); // center

    writeColorParam(x, "fontcolor", "Font Color", color);

    if (useOutline) {
        writeColorParam(x, "linecolor", "Line Color", outlineColor);
        writeNumParam(x, "linewidth", "Line Width", fcpOutline);
        writeNumParam(x, "linesoft", "Line Softness", 0);
    }

    x.writeEndElement(); // effect
}

} // namespace

bool PremiereXmlExporter::exportXml(const Project &project,
                                    const QString &outputPath,
                                    QString *errorOut)
{
    if (project.totalFrames() <= 0) {
        if (errorOut) *errorOut = "No frames to export.";
        return false;
    }
    if (project.mediaFilePath().isEmpty()) {
        if (errorOut) *errorOut = "No media file set.";
        return false;
    }

    const QString mediaAbsPath = QFileInfo(project.mediaFilePath()).absoluteFilePath();
    SourceMediaInfo src = probeSource(mediaAbsPath);
    if (src.width <= 0 || src.height <= 0) {
        if (errorOut) *errorOut = "Cannot probe source media dimensions.";
        return false;
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorOut) *errorOut = "Cannot open output file for writing.";
        return false;
    }

    const int SEQ_W = 1080;
    const int SEQ_H = 1920;
    const RateInfo seqRate{60, false};

    const double srcFps = project.sourceFrameRate() > 0 ? project.sourceFrameRate() : 30.0;
    const double fpsRatio = 60.0 / srcFps;

    auto toSeq = [&](qint64 srcFrame) -> qint64 {
        return static_cast<qint64>(std::round(srcFrame * fpsRatio));
    };

    const qint64 seqDur      = toSeq(project.totalFrames());
    const qint64 seqTrimIn   = toSeq(project.trimStartFrame());
    const qint64 seqTrimOut  = toSeq(project.trimEndFrame());
    const qint64 seqFileDur  = toSeq(project.totalSourceFrames());

    const QString mediaName = QFileInfo(mediaAbsPath).fileName();
    const QString mediaUrl  = toFileUrl(mediaAbsPath);

    // foreground: scale to fit width (match SubtitleRenderer::renderFrame)
    double fgScale = (static_cast<double>(SEQ_W) / src.width) * 100.0;
    // background: scale to fill
    double bgScale = std::max(static_cast<double>(SEQ_W) / src.width,
                              static_cast<double>(SEQ_H) / src.height) * 100.0;

    const QString fileId     = "file-1";
    const QString masterClip = "masterclip-1";
    const QString vidBgId    = "clipitem-vbg";
    const QString vidFgId    = "clipitem-vfg";
    const QString audLId     = "clipitem-aL";
    const QString audRId     = "clipitem-aR";
    const bool stereo = (src.audioChannels >= 2);

    QXmlStreamWriter x(&outFile);
    x.setAutoFormatting(true);
    x.setAutoFormattingIndent(2);
    x.writeStartDocument();
    x.writeDTD("<!DOCTYPE xmeml>");

    x.writeStartElement("xmeml");
    x.writeAttribute("version", "4");

    x.writeStartElement("sequence");
    x.writeAttribute("id", "sequence-1");
    x.writeTextElement("name", "ReelsForge Export");
    x.writeTextElement("duration", QString::number(seqDur));
    writeRate(x, seqRate);
    writeTimecode(x, seqRate);
    x.writeTextElement("in", "-1");
    x.writeTextElement("out", "-1");

    x.writeStartElement("media");

    // ==================== VIDEO ====================
    x.writeStartElement("video");

    x.writeStartElement("format");
    x.writeStartElement("samplecharacteristics");
    x.writeTextElement("width",  QString::number(SEQ_W));
    x.writeTextElement("height", QString::number(SEQ_H));
    x.writeTextElement("anamorphic", "FALSE");
    x.writeTextElement("pixelaspectratio", "square");
    x.writeTextElement("fielddominance", "none");
    writeRate(x, seqRate);
    x.writeTextElement("colordepth", "24");
    x.writeEndElement(); // samplecharacteristics
    x.writeEndElement(); // format

    auto writeVideoClip = [&](const QString &clipId, bool isFirstDef) {
        x.writeTextElement("masterclipid", masterClip);
        x.writeTextElement("name", mediaName);
        x.writeTextElement("enabled", "TRUE");
        x.writeTextElement("duration", QString::number(seqFileDur));
        writeRate(x, seqRate);
        x.writeTextElement("start", "0");
        x.writeTextElement("end",   QString::number(seqDur));
        x.writeTextElement("in",    QString::number(seqTrimIn));
        x.writeTextElement("out",   QString::number(seqTrimOut));

        if (isFirstDef) {
            x.writeStartElement("file");
            x.writeAttribute("id", fileId);
            x.writeTextElement("name", mediaName);
            x.writeTextElement("pathurl", mediaUrl);
            writeRate(x, seqRate);
            x.writeTextElement("duration", QString::number(seqFileDur));
            x.writeStartElement("media");
            x.writeStartElement("video");
            x.writeTextElement("duration", QString::number(seqFileDur));
            x.writeStartElement("samplecharacteristics");
            x.writeTextElement("width",  QString::number(src.width));
            x.writeTextElement("height", QString::number(src.height));
            x.writeEndElement();
            x.writeEndElement(); // video
            x.writeStartElement("audio");
            x.writeTextElement("duration", QString::number(seqFileDur));
            x.writeStartElement("samplecharacteristics");
            x.writeTextElement("depth", "16");
            x.writeTextElement("samplerate", "48000");
            x.writeEndElement();
            x.writeEndElement(); // audio
            x.writeEndElement(); // media
            x.writeEndElement(); // file
        } else {
            x.writeStartElement("file");
            x.writeAttribute("id", fileId);
            x.writeEndElement();
        }
    };

    // ── V1: background (scale to fill + opacity 35%) ──
    x.writeStartElement("track");
    {
        x.writeStartElement("clipitem");
        x.writeAttribute("id", vidBgId);
        writeVideoClip(vidBgId, true);
        writeScaleFilter(x, bgScale);
        writeOpacityFilter(x, 35.0);
        x.writeEndElement(); // clipitem
    }
    x.writeEndElement(); // track V1

    // ── V2: foreground (scale to fit width) ──
    x.writeStartElement("track");
    {
        x.writeStartElement("clipitem");
        x.writeAttribute("id", vidFgId);
        writeVideoClip(vidFgId, false);
        writeScaleFilter(x, fgScale);

        writeLink(x, vidFgId, "video", 2, 1);
        writeLink(x, audLId,  "audio", 1, 1);
        if (stereo) writeLink(x, audRId, "audio", 2, 1);

        x.writeEndElement(); // clipitem
    }
    x.writeEndElement(); // track V2

    // ── V3: title ──
    const VideoTitle &title = project.videoTitle();
    if (!title.text.trimmed().isEmpty()) {
        x.writeStartElement("track");
        x.writeStartElement("generatoritem");
        x.writeAttribute("id", "genitem-title");
        x.writeTextElement("name", title.text.left(30).trimmed());
        x.writeTextElement("enabled", "TRUE");
        x.writeTextElement("duration", QString::number(seqDur));
        writeRate(x, seqRate);
        x.writeTextElement("start", "0");
        x.writeTextElement("end",   QString::number(seqDur));
        x.writeTextElement("in", "0");
        x.writeTextElement("out", QString::number(seqDur));

        writeTextGeneratorEffect(x, title.text.trimmed(),
                                 title.font, title.fontSize, SEQ_H,
                                 title.color,
                                 title.outlineEnabled, title.outlineColor,
                                 title.outlineWidthPx);

        x.writeEndElement(); // generatoritem
        x.writeEndElement(); // track V3
    }

    // ── V4: subtitles ──
    const auto &subs = project.subtitles();
    bool hasSubs = false;
    for (const auto &s : subs)
        if (!s.text.trimmed().isEmpty()) { hasSubs = true; break; }

    if (hasSubs) {
        x.writeStartElement("track");

        QFont  subFont  = project.globalSubtitleFont();
        float  subSize  = project.globalSubtitleFontSize();
        subFont.setPointSizeF(subSize > 0 ? subSize : 48.0);
        const QColor &subColor = project.globalSubtitleColor();
        bool          subOL    = project.globalSubtitleOutlineEnabled();
        const QColor &subOLc   = project.globalSubtitleOutlineColor();
        float         subOLw   = project.globalSubtitleOutlineWidthPx();

        int genIdx = 0;
        for (int i = 0; i < subs.size(); ++i) {
            const SubtitleEntry &sub = subs[i];
            if (sub.text.trimmed().isEmpty()) continue;

            qint64 sf = toSeq(sub.keyFrame);
            qint64 ef = (i + 1 < subs.size()) ? toSeq(subs[i + 1].keyFrame) : seqDur;
            qint64 dur = ef - sf;
            if (dur <= 0) continue;

            ++genIdx;
            QString gid = QString("genitem-sub-%1").arg(genIdx);

            x.writeStartElement("generatoritem");
            x.writeAttribute("id", gid);
            x.writeTextElement("name", sub.text.left(30).trimmed());
            x.writeTextElement("enabled", "TRUE");
            x.writeTextElement("duration", QString::number(dur));
            writeRate(x, seqRate);
            x.writeTextElement("start", QString::number(sf));
            x.writeTextElement("end",   QString::number(ef));
            x.writeTextElement("in", "0");
            x.writeTextElement("out", QString::number(dur));

            writeTextGeneratorEffect(x, sub.text,
                                     subFont,
                                     subSize > 0 ? subSize : 48.0, SEQ_H,
                                     subColor,
                                     subOL, subOLc, subOLw);

            x.writeEndElement(); // generatoritem
        }
        x.writeEndElement(); // track V4
    }

    x.writeEndElement(); // video

    // ==================== AUDIO ====================
    x.writeStartElement("audio");
    x.writeTextElement("numOutputChannels", stereo ? "2" : "1");

    x.writeStartElement("format");
    x.writeStartElement("samplecharacteristics");
    x.writeTextElement("depth", "16");
    x.writeTextElement("samplerate", "48000");
    x.writeEndElement();
    x.writeEndElement(); // format

    auto writeAudioClip = [&](const QString &clipId, int srcTrack) {
        x.writeStartElement("clipitem");
        x.writeAttribute("id", clipId);
        x.writeTextElement("masterclipid", masterClip);
        x.writeTextElement("name", mediaName);
        x.writeTextElement("enabled", "TRUE");
        x.writeTextElement("duration", QString::number(seqFileDur));
        writeRate(x, seqRate);
        x.writeTextElement("start", "0");
        x.writeTextElement("end",   QString::number(seqDur));
        x.writeTextElement("in",    QString::number(seqTrimIn));
        x.writeTextElement("out",   QString::number(seqTrimOut));

        x.writeStartElement("file");
        x.writeAttribute("id", fileId);
        x.writeEndElement();

        x.writeStartElement("sourcetrack");
        x.writeTextElement("mediatype", "audio");
        x.writeTextElement("trackindex", QString::number(srcTrack));
        x.writeEndElement();

        writeLink(x, vidFgId, "video", 2, 1);
        writeLink(x, audLId,  "audio", 1, 1);
        if (stereo) writeLink(x, audRId, "audio", 2, 1);

        x.writeEndElement(); // clipitem
    };

    x.writeStartElement("track");
    writeAudioClip(audLId, 1);
    x.writeEndElement();

    if (stereo) {
        x.writeStartElement("track");
        writeAudioClip(audRId, 2);
        x.writeEndElement();
    }

    x.writeEndElement(); // audio

    x.writeEndElement(); // media
    x.writeEndElement(); // sequence
    x.writeEndElement(); // xmeml
    x.writeEndDocument();

    outFile.close();
    return true;
}
