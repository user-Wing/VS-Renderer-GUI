# Geometry and timeline snapshot from the analysis canvas; GUI chrome is omitted.
import vapoursynth as vs
import math
import sys
core = vs.core
sources = [core.ffms2.Source(source=s['path']) for s in spec['sources']]
fpsnum, fpsden = sources[0].fps_num, sources[0].fps_den
fps = fpsnum / fpsden
length = max(1, int(math.floor(spec['duration'] * fps)))
width, height = spec['width'], spec['height']

def convert(clip):
    if clip.format.color_family == vs.RGB:
        return core.resize.Point(clip, format=vs.RGBS)
    params = {}
    chroma = spec['chroma']
    if chroma == 2: params = dict(filter_param_a=0, filter_param_b=.5)
    if chroma == 7: params = dict(filter_param_a=1, filter_param_b=0)
    if chroma == 8: params = dict(filter_param_a=1/3, filter_param_b=1/3)
    kernel = {0:'Point',1:'Bilinear',2:'Bicubic',3:'Lanczos',5:'Spline36',7:'Bicubic',8:'Bicubic'}[chroma]
    # Use source matrix/range props where available, BT.709 as the missing-prop fallback.
    props = clip.get_frame(0).props
    matrix = props.get('_Matrix', 1)
    if matrix in (0,2): matrix = 1
    return getattr(core.resize,kernel)(clip,format=vs.RGBS,matrix_in=matrix,range_in=props.get('_ColorRange',1),**params)

def aligned(source, offset):
    source_fps = source.fps_num/source.fps_den
    template = core.std.BlankClip(source,length=length,fpsnum=fpsnum,fpsden=fpsden)
    def select(n):
        idx = max(0,min(source.num_frames-1,int(math.floor((n/fps+offset)*source_fps+.000001))))
        return source[idx] * length
    return core.std.FrameEval(template,eval=select)

def render(source, item):
    x0,y0,x1,y1 = item['region']
    x0,x1 = round(x0*width),round(x1*width)
    y0,y1 = round(y0*height),round(y1*height)
    tile_width,tile_height = x1-x0,y1-y0
    if spec['wipe']:
        vw,vh = width,height
        cutx,cuty=x0,y0
    else:
        vw,vh=tile_width,tile_height
        cutx,cuty=0,0
    # Fit in the preview's logical viewport, then map to the export dimensions.
    preview_w,preview_h = item['viewport']
    fit = min(preview_w/source.width,preview_h/source.height)*item['zoom']
    image_w,image_h=source.width*fit,source.height*fit
    px=(preview_w-image_w)/2+item['pan'][0]*max(0,image_w-preview_w)/2
    py=(preview_h-image_h)/2+item['pan'][1]*max(0,image_h-preview_h)/2
    left,top=px*vw/preview_w,py*vh/preview_h
    image_w,image_h=image_w*vw/preview_w,image_h*vh/preview_h
    a=max(cutx,math.ceil(left));b=min(cutx+tile_width,math.floor(left+image_w))
    c=max(cuty,math.ceil(top));d=min(cuty+tile_height,math.floor(top+image_h))
    blank=core.std.BlankClip(source,width=tile_width,height=tile_height,color=[0,0,0])
    if b>a and d>c:
        scale_x,scale_y=source.width/image_w,source.height/image_h
        kernel={0:'Point',1:'Bilinear',2:'Bicubic',3:'Lanczos',5:'Spline36'}[spec['scaler']]
        params=dict(filter_param_a=0,filter_param_b=.5) if spec['scaler']==2 else {}
        image=getattr(core.resize,kernel)(source,width=b-a,height=d-c,src_left=(a-left)*scale_x,src_top=(c-top)*scale_y,src_width=(b-a)*scale_x,src_height=(d-c)*scale_y,**params)
        tile=core.std.AddBorders(image,left=a-cutx,right=cutx+tile_width-b,top=c-cuty,bottom=cuty+tile_height-d,color=[0,0,0])
    else: tile=blank
    return (x0,y0,x1,y1),tile

parts=[render(aligned(convert(source),item['offset']),item) for source,item in zip(sources,spec['sources'])]
# Stack cropped tiles so nine-way 4K export does not allocate nine full RGB canvases.
ys=sorted(set([0,height]+[rect[1] for rect,_ in parts]+[rect[3] for rect,_ in parts]))
rows=[]
for y0,y1 in zip(ys,ys[1:]):
    strips=[]
    cursor=0
    for (x0,top,x1,bottom),tile in sorted(parts,key=lambda p:p[0][0]):
        if top<=y0 and bottom>=y1:
            if x0>cursor: strips.append(core.std.BlankClip(parts[0][1],width=x0-cursor,height=y1-y0,color=[0,0,0]))
            strips.append(core.std.CropAbs(tile,width=x1-x0,height=y1-y0,left=0,top=y0-top))
            cursor=x1
    if cursor<width: strips.append(core.std.BlankClip(parts[0][1],width=width-cursor,height=y1-y0,color=[0,0,0]))
    rows.append(core.std.StackHorizontal(strips) if len(strips)>1 else strips[0])
clip=core.std.StackVertical(rows) if len(rows)>1 else rows[0]
clip=core.resize.Bicubic(clip,format=vs.YUV444P16,matrix_s='709',range_s='limited',dither_type='error_diffusion')
print('__VSR_DURATION__='+str(length/fps),file=sys.stderr,flush=True)
clip.set_output(0)
