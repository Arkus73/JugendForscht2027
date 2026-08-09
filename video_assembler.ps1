param(
    [int]$FPS,
    [string]$Name
)
cd output
ffmpeg -framerate $FPS -i image%03d.png -vf "pad=ceil(iw/2)*2:ceil(ih/2)*2" -c:v libx264 -preset slow -crf 0 -tune stillimage -pix_fmt yuv444p $Name
cd ..