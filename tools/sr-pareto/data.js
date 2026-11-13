/* Shared by the browser and Node's focused data checks; no runtime dependencies. */
(function (root) {
  function validate(data) {
    if (!data || typeof data !== "object" || Array.isArray(data)) throw new Error("JSON 顶层必须是对象。");
    if (!Array.isArray(data.algorithms)) throw new Error("缺少 algorithms 数组。");
    for (const field of ["title", "description", "xLabel", "yLabel"]) {
      if (data[field] !== undefined && typeof data[field] !== "string") throw new Error(`${field} 必须是字符串。`);
    }
    for (const field of ["yMin", "yMax"]) {
      if (data[field] !== undefined && !Number.isFinite(data[field])) throw new Error(`${field} 必须是有限数值。`);
    }
    data.algorithms.forEach((point, index) => {
      const prefix = `第 ${index + 1} 个算法`;
      if (!point || typeof point.name !== "string" || !point.name.trim()) throw new Error(`${prefix} 缺少有效 name。`);
      if (!Number.isFinite(point.compute) || point.compute < 1 || point.compute > 10000) throw new Error(`${prefix} 的 compute 必须是 1–10000 之间的原始数值。`);
      if (!Number.isFinite(point.quality)) throw new Error(`${prefix} 的 quality 必须是有限数值。`);
      if (point.group !== undefined && (typeof point.group !== "string" || !point.group.trim())) throw new Error(`${prefix} 的 group 必须是非空字符串。`);
      if (point.preview !== undefined && typeof point.preview !== "boolean") throw new Error(`${prefix} 的 preview 必须是布尔值 true 或 false。`);
    });
    const values = data.algorithms.map(point => point.quality);
    const low = values.length ? Math.min(...values) : 0;
    const high = values.length ? Math.max(...values) : 100;
    const padding = Math.max((high - low) * .1, 1);
    const yMin = data.yMin ?? low - padding;
    const yMax = data.yMax ?? high + padding;
    if (yMin >= yMax || !Number.isFinite(yMax - yMin)) throw new Error("yMax 必须大于 yMin，且跨度必须是有限数值。");
    if (values.some(value => value < yMin || value > yMax)) throw new Error("效果值超出 yMin/yMax，请扩大纵轴范围或删除范围设置以自动适配。");
    return { ...data, yMin, yMax };
  }

  function frontier(points) {
    const eligible = points.filter(point => !point.preview);
    return eligible.filter(point => !eligible.some(other =>
      other.compute <= point.compute && other.quality >= point.quality &&
      (other.compute < point.compute || other.quality > point.quality)
    )).sort((a, b) => a.compute - b.compute || b.quality - a.quality);
  }

  const api = { validate, frontier, logPosition: compute => Math.log10(compute) / 4 };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.ParetoData = api;
})(typeof window === "undefined" ? globalThis : window);
