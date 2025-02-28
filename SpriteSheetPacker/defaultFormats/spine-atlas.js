

function exportSpriteSheet(dataFilePath, imageFilePath, spriteFrames, textureSize)
{
    var contents = "";
    contents += getFileName(imageFilePath) + "\n";
    contents += "size: " + textureSize.width + ", " + textureSize.height + "\n";
    contents += "format: RGBA8888\n";
    contents += "filter: Linear,Linear\n";
    contents += "repeat: none\n";
    
    for(var key in spriteFrames)
    {
        var spriteFrame = spriteFrames[key];
        contents += removeExtension(key) + "\n";
        contents += "  rotate: " + spriteFrame.rotated + "\n";
        contents += "  xy: " + spriteFrame.frame.x + ", " + spriteFrame.frame.y + "\n";
        contents += "  size: " + spriteFrame.sourceColorRect.width + ", " + spriteFrame.sourceColorRect.height + "\n";
        contents += "  orig: " + spriteFrame.sourceSize.width + ", " + spriteFrame.sourceSize.height + "\n";
        contents += "  offset: " + spriteFrame.offset.x + ", " + spriteFrame.offset.y + "\n";
        contents += "  index: -1\n";
    }
    contents += "\n";
    
    return {
        data: contents,
        format: "atlas"
    };
}


function getFileName(str)
{
    return str.split('\\').pop().split('/').pop();
}

function removeExtension(filename)
{
    return filename.substring(0, filename.lastIndexOf('.')) || filename;
}