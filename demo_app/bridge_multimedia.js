ImportModule("Skia"); 

const canvas = new Skia.Canvas(512, 512);

canvas.sweepGradient(51, 21, 65, [
    {
        color: 3
    }
])

const surface = canvas.toSurface();

const jpeg = surface.encodeJPEG(100); 