#include "agent/AgentImageMessages.h"
#include <QRegularExpression>

namespace vsr::agent_images {
bool providerSupportsImages(const AgentProvider &provider)
{
    const auto capability=provider.modelCapability;const auto endpoint=capability.value("endpoint").toString();
    if(!capability.value("imageInputSupported").toBool() || endpoint.isEmpty() || capability.value("model").toString()!=provider.model)return false;
    auto bound=provider;bound.baseUrl=endpoint;const auto url=provider.completionUrl();return url.isValid() && !url.host().isEmpty() && bound.completionUrl()==url;
}

bool validDataUrl(const QString &url,QString *mime)
{
    if(url.isEmpty() || url.size()>maximumEncodedBytes)return false;
    static const QRegularExpression pattern(QStringLiteral("\\Adata:(image/(?:png|jpeg|webp));base64,([A-Za-z0-9+/]+={0,2})\\z"));
    const auto match=pattern.match(url);if(!match.hasMatch())return false;const auto encoded=match.captured(2).toLatin1();
    if(encoded.size()%4)return false;const auto decoded=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
    if(!decoded || decoded.decoded.isEmpty() || decoded.decoded.toBase64()!=encoded)return false;
    if(mime)*mime=match.captured(1);return true;
}

bool validContent(const QJsonArray &blocks)
{
    int images=0;if(blocks.isEmpty())return false;
    for(const auto &value:blocks){
        if(!value.isObject())return false;const auto block=value.toObject();const auto type=block.value("type").toString();
        if(type=="text"){if(block.size()!=2 || !block.value("text").isString())return false;}
        else if(type=="image_url"){
            if(block.size()!=2 || !block.value("image_url").isObject() || ++images>maximumImages)return false;
            const auto image=block.value("image_url").toObject();if(!image.value("url").isString() || !validDataUrl(image.value("url").toString()))return false;
            if(image.size()!=1){const auto detail=image.value("detail").toString();if(image.size()!=2 || !image.contains("detail") || (detail!="auto" && detail!="low" && detail!="high"))return false;}
        }else return false;
    }
    return images>0;
}

QString textContent(const QJsonValue &content,int *images)
{
    if(content.isString())return content.toString();QStringList parts;
    for(const auto &value:content.toArray()){
        const auto block=value.toObject();if(block.value("type")==QJsonValue("text"))parts.append(block.value("text").toString());
        else if(block.value("type")==QJsonValue("image_url")){
            if(images)++*images;const auto url=block.value("image_url").toObject().value("url").toString();parts.append(QStringLiteral("[图像 %1；图像数据未包含在文字摘要中]").arg(url.section(';',0,0).mid(5)));
        }
    }
    return parts.join('\n');
}

QJsonArray textOnly(const QJsonArray &messages,int *images)
{
    QJsonArray result;for(const auto &value:messages){auto message=value.toObject();if(message.value("content").isArray())message.insert("content",textContent(message.value("content"),images));result.append(message);}return result;
}

bool containsImages(const QJsonArray &messages)
{
    for(const auto &value:messages)for(const auto &block:value.toObject().value("content").toArray())if(block.toObject().value("type")==QJsonValue("image_url"))return true;return false;
}
}
