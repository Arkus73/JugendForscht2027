param(
    [int]$FPS,
    [string]$Name
)
cd output
ffmpeg -framerate $FPS -i image%03d.png -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" -c:v libx264 -pix_fmt yuv420p $Name
cd ..