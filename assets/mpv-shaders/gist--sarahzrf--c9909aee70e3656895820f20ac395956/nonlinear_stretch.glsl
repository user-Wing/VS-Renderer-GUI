//!HOOK MAINPRESUB
//!BIND HOOKED

const float videoNonLinStretchRatio = 1;

vec2 stretch(vec2 pos, float par) {
  float m_stretch = pow(par, videoNonLinStretchRatio);

  float x = pos.x - 0.5;
  return vec2(mix(x * abs(x) * 2.0, x, m_stretch) + 0.5, pos.y);
}

vec4 hook() {
  float dar = target_size.x / target_size.y,
        sar = HOOKED_size.x / HOOKED_size.y,
        par = dar / sar;

  return HOOKED_tex(stretch(HOOKED_pos, par));
}

