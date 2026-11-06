#pragma once
#include <QString>

namespace vsr {
struct ShaderPackageResult {
    QString packagePath;
    QString error;
};

class ShaderPackage final {
public:
    // HLSL: main/ps_5_0 at the requested output size, t0/s0,
    // optional b0={sourceSize, outputSize} (two float4, xy=size, zw=reciprocal).
    // Multi-pass HLSL: MAIN/PREKERNEL //! directives, standalone main per block.
    // GLSL: mpv MAIN/PREKERNEL pixel hooks; unsupported semantics return an error.
    static ShaderPackageResult prepare(const QString &sourcePath, const QString &cacheDirectory = {});
};
}
