const test = require("node:test");
const assert = require("node:assert/strict");
const { validate, frontier, logPosition } = require("./data.js");
const point = (name, compute, quality) => ({ name, compute, quality });

test("frontier handles unsorted input, dominance, equal costs/scores and exact ties", () => {
  const points = [point("dominated", 90, 30), point("B", 10, 50), point("same cost", 10, 40), point("A", 1, 20), point("same score", 15, 50), point("tie", 10, 50), point("C", 10000, 80)];
  assert.deepEqual(frontier(points).map(p => p.name), ["A", "B", "tie", "C"]);
  assert.equal(points[0].name, "dominated");
});
test("each decade occupies the same distance, with exact domain endpoints", () => {
  assert.deepEqual([1, 10, 100, 1000, 10000].map(logPosition), [0, .25, .5, .75, 1]);
});
test("valid data supports decimals, negative quality and automatic y bounds", () => {
  const data = validate({ algorithms: [point("A", 1.5, -2), point("B", 10000, -.5)] });
  assert.ok(data.yMin < -2 && data.yMax > -.5);
  assert.equal(validate({ algorithms: [point("constant", 5, 20)] }).yMin, 19);
});
test("empty data and a single algorithm have defined behavior", () => {
  assert.deepEqual(frontier([]), []);
  assert.ok(validate({ algorithms: [] }).yMax > 0);
  assert.equal(frontier([point("A", 1, 0)]).length, 1);
});
test("invalid data is rejected rather than omitted or clamped", () => {
  for (const compute of [0, -1, 10001, "10", null, NaN, Infinity]) {
    assert.throws(() => validate({ algorithms: [point("A", compute, 20)] }), /compute/);
  }
  for (const quality of [undefined, "80", null, NaN, Infinity]) {
    assert.throws(() => validate({ algorithms: [point("A", 10, quality)] }), /quality/);
  }
  for (const invalid of [null, [], {}, { algorithms: [null] }, { algorithms: [point(" ", 1, 2)] }, { title: 2, algorithms: [] }, { algorithms: [], yMin: 5, yMax: 5 }, { algorithms: [], yMax: "100" }, { algorithms: [point("A", 1, 120)], yMax: 100 }]) {
    assert.throws(() => validate(invalid));
  }
});
test("editable JSON validates without requiring a fixed dataset size", () => {
  assert.doesNotThrow(() => validate(require("./algorithms.json")));
});
test("preview models neither enter the frontier nor dominate released algorithms", () => {
  const released = [point("A", 10, 50), { ...point("B", 100, 80), preview: false }];
  const preview = { ...point("Preview", 1, 100), preview: true };
  assert.deepEqual(frontier([...released, preview]), released);
  assert.deepEqual(frontier([preview]), []);
  assert.equal(validate({ algorithms: [preview] }).algorithms[0].preview, true);
});
test("preview must be a boolean, and omitted flags remain valid", () => {
  for (const preview of ["true", "false", 0, 1, null]) {
    assert.throws(() => validate({ algorithms: [{ ...point("A", 10, 50), preview }] }), /preview/);
  }
  assert.doesNotThrow(() => validate({ algorithms: [point("A", 10, 50), { ...point("B", 20, 60), preview: false }] }));
});
