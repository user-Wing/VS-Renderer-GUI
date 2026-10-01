def _vsr_sharpen_shader(mode, strength, threshold):
    weights = 'float[9](0,1,0,1,4,1,0,1,0)' if mode == 'crispen_edges' else 'float[9](1,2,1,2,4,2,1,2,1)'
    divisor = 8 if mode == 'crispen_edges' else 16
    detail = '''float wide=0.0; float w[5]=float[5](1,4,6,4,1);
    for(int y=-2;y<=2;y++) for(int x=-2;x<=2;x++)
        wide+=HOOKED_texOff(vec2(x,y)).r*w[x+2]*w[y+2]/256.0;''' if mode == 'enhance_detail' else 'float wide=blur;'
    formula = 'center+(center-blur+(blur-wide)*0.5)*STRENGTH' if mode == 'enhance_detail' else 'center+(center-blur)*STRENGTH'
    if mode == 'sharpen_edges':
        formula = '(abs(center-blur)>THRESHOLD ? '+formula+' : center)'
    shader = '''//!HOOK LUMA
//!BIND HOOKED
//!DESC VS Renderer GPU sharpen
vec4 hook() {
vec4 pixel=HOOKED_tex(HOOKED_pos); float center=pixel.r;
float lo=center,hi=center,blur=0.0; float weights[9]=WEIGHTS;
for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {
    float v=HOOKED_texOff(vec2(x,y)).r;
    lo=min(lo,v);hi=max(hi,v);blur+=v*weights[(y+1)*3+x+1]/DIVISOR;
}
DETAIL
pixel.r=clamp(FORMULA,lo,hi); return pixel;
}
'''.replace('WEIGHTS',weights).replace('DIVISOR',str(float(divisor))).replace('DETAIL',detail).replace('FORMULA',formula).replace('STRENGTH',str(float(strength))).replace('THRESHOLD',str(float(threshold)*256/65535))
    return shader

def _vsr_sharpen_chain(c, stages):
    if c.format.color_family != vs.YUV:
        for mode, strength, threshold in stages:
            c = _vsr_sharpen(c, mode, strength, threshold)
        return c
    c=core.resize.Point(c,format=core.query_video_format(vs.YUV,vs.INTEGER,16,c.format.subsampling_w,c.format.subsampling_h).id)
    shader='\n'.join(_vsr_sharpen_shader(*stage) for stage in stages)
    filtered=core.placebo.Shader(c,shader_s=shader,width=c.width,height=c.height,linearize=False,sigmoidize=False)
    return core.std.ShufflePlanes([filtered,c,c],planes=[0,1,2],colorfamily=vs.YUV)

def _vsr_sharpen(c, mode, strength, threshold=0):
    if c.format.color_family == vs.YUV:
        return _vsr_sharpen_chain(c, [(mode, strength, threshold)])
    matrix=[0,1,0,1,4,1,0,1,0] if mode=='crispen_edges' else [1,2,1,2,4,2,1,2,1]
    blur=core.std.Convolution(c,matrix=matrix,planes=[0])
    scale=1/255 if c.format.sample_type==vs.FLOAT else 1<<max(0,c.format.bits_per_sample-8)
    if mode=='enhance_detail':
        wide=core.std.Convolution(blur,matrix=[1,2,1,2,4,2,1,2,1],planes=[0])
        sharp=core.std.Expr([c,blur,wide],expr=['x x y - y z - 0.5 * + '+str(strength)+' * +']+['']*(c.format.num_planes-1))
    else:
        expr='x x y - '+str(strength)+' * +'
        if mode=='sharpen_edges': expr='x y - abs '+str(threshold*scale)+' > '+expr+' x ?'
        sharp=core.std.Expr([c,blur],expr=[expr]+['']*(c.format.num_planes-1))
    return core.std.Expr([sharp,core.std.Minimum(c,planes=[0]),core.std.Maximum(c,planes=[0])],expr=['x y max z min']+['']*(c.format.num_planes-1))
