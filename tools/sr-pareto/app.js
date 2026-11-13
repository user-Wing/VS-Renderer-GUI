(() => {
  const $ = id => document.getElementById(id);
  const palette = ["#70c7ab", "#80b4e0", "#dcaa75", "#b298dc", "#ce8586", "#aabf77"];
  const ns = "http://www.w3.org/2000/svg";
  const fmt = value => value.toLocaleString("zh-CN", { maximumFractionDigits: 20 });
  let data = null;
  let source = null;
  let generation = 0;

  function svgNode(tag, attributes = {}, text = "", parent = $("chart")) {
    const node = document.createElementNS(ns, tag);
    Object.entries(attributes).forEach(([key, value]) => node.setAttribute(key, value));
    if (text) node.textContent = text;
    parent.append(node);
    return node;
  }

  function render() {
    const chart = $("chart");
    chart.replaceChildren();
    svgNode("title", { id: "chart-title" }, data?.title || "超分算法质量与算力");
    svgNode("desc", { id: "chart-description" }, "横轴为 1 到 10000 的对数算力，纵轴为效果值。阶梯线表示正式算法的 Pareto 前沿；Preview 模型不计入前沿。算法名称与原始数值可通过聚焦点查看。");
    const allPoints = data?.algorithms || [];
    const points = allPoints.filter(point => !$("hide-preview").checked || !point.preview);
    const frontier = ParetoData.frontier(allPoints);
    const optimal = new Set(frontier);
    const groups = [...new Set(allPoints.map(point => point.group || "未分组"))];
    const color = point => palette[groups.indexOf(point.group || "未分组") % palette.length];
    const left = 85, right = 1060, top = 30, bottom = 565;
    const yMin = data?.yMin ?? 0, yMax = data?.yMax ?? 100;
    const x = value => left + ParetoData.logPosition(value) * (right - left);
    const y = value => bottom - (value - yMin) / (yMax - yMin) * (bottom - top);

    for (let decade = 0; decade < 4; decade++) {
      for (const multiple of [2, 5]) {
        const xx = x(multiple * 10 ** decade);
        svgNode("line", { x1: xx, x2: xx, y1: top, y2: bottom, class: "minor-grid" });
      }
    }
    for (const value of [1, 10, 100, 1000, 10000]) {
      svgNode("line", { x1: x(value), x2: x(value), y1: top, y2: bottom, class: "grid" });
      svgNode("text", { x: x(value), y: bottom + 28, "text-anchor": "middle" }, fmt(value));
    }
    for (let index = 0; index <= 5; index++) {
      const value = yMin + (yMax - yMin) * index / 5;
      svgNode("line", { x1: left, x2: right, y1: y(value), y2: y(value), class: "grid" });
      svgNode("text", { x: left - 15, y: y(value) + 5, "text-anchor": "end" }, fmt(Number(value.toPrecision(6))));
    }
    svgNode("text", { x: (left + right) / 2, y: 636, "text-anchor": "middle", class: "axis-label" }, data?.xLabel || "处理算力要求（原始值 · 对数刻度）");
    svgNode("text", { transform: `translate(22 ${(top + bottom) / 2}) rotate(-90)`, "text-anchor": "middle", class: "axis-label" }, data?.yLabel || "处理效果");
    if (frontier.length) {
      let path = `M ${x(frontier[0].compute)} ${y(frontier[0].quality)}`;
      frontier.slice(1).forEach(point => { path += ` H ${x(point.compute)} V ${y(point.quality)}`; });
      svgNode("path", { d: path + ` H ${right}`, class: "frontier" });
    }
    const labelLayer = svgNode("g", { "aria-hidden": "true" });
    const pointLayer = svgNode("g");
    const occupied = points.map(point => ({ x: x(point.compute) - 8, y: y(point.quality) - 8, width: 16, height: 16 }));
    const overlaps = (a, b) => a.x < b.x + b.width + 4 && a.x + a.width + 4 > b.x && a.y < b.y + b.height + 3 && a.y + a.height + 3 > b.y;
    points.forEach(point => {
      const cx = x(point.compute), cy = y(point.quality);
      const info = `${point.name}；算力 ${fmt(point.compute)}；效果 ${fmt(point.quality)}；${point.group || "未分组"}；${point.preview ? "Preview 阶段，不计入前沿" : optimal.has(point) ? "位于 Pareto 前沿" : "非前沿算法"}`;
      const group = svgNode("g", { class: `point${optimal.has(point) ? " optimal" : ""}${point.preview ? " preview" : ""}`, tabindex: "0", role: "img", "aria-label": info }, "", pointLayer);
      svgNode("title", {}, info, group);
      svgNode("circle", { cx, cy, r: optimal.has(point) ? 6.5 : 5.5, fill: color(point) }, "", group);
      if (!$("labels").checked) return;
      const label = svgNode("text", { class: `point-label${optimal.has(point) ? " optimal" : ""}` }, point.name + (point.preview ? " (Preview)" : ""), labelLayer);
      const width = label.getComputedTextLength();
      let best, bestScore = Infinity;
      for (const offset of [-14, 24, -32, 42, -50, 60, -68, 78, -86, 96]) {
        for (const xx of [cx + 12, cx - width - 12, cx - width / 2]) {
          const box = { x: Math.max(left, Math.min(right - width, xx)), y: Math.max(top, Math.min(bottom - 15, cy + offset - 12)), width, height: 15 };
          const score = occupied.filter(other => overlaps(box, other)).length * 10000 + Math.abs(box.y + 12 - cy) + Math.abs(box.x - cx) * .05;
          if (score < bestScore) { best = box; bestScore = score; }
        }
      }
      label.setAttribute("x", best.x);
      label.setAttribute("y", best.y + 12);
      occupied.push(best);
      if (Math.abs(best.y + 6 - cy) > 30) {
        const leader = svgNode("line", { x1: cx, y1: cy, x2: Math.max(best.x, Math.min(best.x + width, cx)), y2: best.y + 7, class: "label-leader" }, "", labelLayer);
        labelLayer.insertBefore(leader, label);
      }
    });
    if (!points.length) svgNode("text", { x: (left + right) / 2, y: (top + bottom) / 2, "text-anchor": "middle" }, allPoints.length ? "当前算法均为 Preview，请取消隐藏以查看" : data ? "暂无算法，请在 JSON 中添加数据" : "打开 JSON，开始比较算法");
    $("legend").replaceChildren();
    groups.forEach((name, index) => {
      const item = document.createElement("div");
      item.className = "legend-item";
      const swatch = document.createElement("span");
      swatch.className = "swatch";
      swatch.style.background = palette[index % palette.length];
      item.append(swatch, document.createTextNode(name));
      $("legend").append(item);
    });
    $("frontier-list").replaceChildren();
    frontier.forEach(point => {
      const item = document.createElement("li");
      const values = document.createElement("small");
      values.textContent = `算力 ${fmt(point.compute)} / 效果 ${fmt(point.quality)}`;
      item.append(document.createTextNode(point.name), values);
      $("frontier-list").append(item);
    });
    $("frontier-count").textContent = frontier.length;
    $("point-count").textContent = `显示 ${points.length}/${allPoints.length} 个算法 · ${frontier.length} 个前沿点`;
  }

  async function load(nextSource) {
    const request = ++generation;
    $("status").className = "";
    $("status").textContent = "正在读取 JSON…";
    try {
      let content;
      if (nextSource.file) content = await nextSource.file.text();
      else {
        const response = await fetch("algorithms.json", { cache: "no-store" });
        if (!response.ok) throw new Error(`读取 algorithms.json 失败（HTTP ${response.status}）。`);
        content = await response.text();
      }
      const parsed = ParetoData.validate(JSON.parse(content.replace(/^\uFEFF/, "")));
      if (request !== generation) return;
      data = parsed;
      source = nextSource;
      $("title").textContent = data.title || "超分算法的质量与算力";
      $("description").textContent = data.description || "用同一把尺子，找到质量与算力之间的取舍。";
      $("filename").textContent = source.file?.name || "algorithms.json";
      render();
      $("status").textContent = `已加载 ${data.algorithms.length} 个算法 · 算力越低越好，效果越高越好。`;
    } catch (error) {
      if (request !== generation) return;
      $("status").className = "error";
      $("status").textContent = `加载失败：${error.message}${data ? " 当前图表保留上次有效数据。" : ""}${nextSource.file ? " 如已修改文件，请重新选择 JSON。" : " 可点击“打开 JSON”选择本地文件。"}`;
    }
  }

  $("file").addEventListener("change", event => {
    const file = event.target.files[0];
    if (file) load({ file });
    event.target.value = "";
  });
  $("reload").addEventListener("click", () => {
    if (source) load(source);
    else if (location.protocol !== "file:") load({});
    else $("file").click();
  });
  $("labels").addEventListener("change", render);
  $("hide-preview").addEventListener("change", render);
  render();
  if (location.protocol !== "file:") load({});
  else $("status").textContent = "请点击“打开 JSON”选择 algorithms.json；双击打开的页面需手动选择数据文件。";
})();
