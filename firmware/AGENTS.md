# firmware

ChipAI 固件侧

上级工程说明见 `../AGENTS.md`。

## 用途与 Target 判定

- 移动端：不适用
- 智能硬件：需要

## 助手约束

- Loop：先定可验证 Done 与停止条件；每切片过验证环；失败回灌；详见 docs/LOOP_ENGINEERING.md
- Harness：遵循根 AGENTS「Harness Engineering」；Guide=本文件，Sensor=测试/CI；同类失误二次须沉淀规则或测试（棘轮）。详见 docs/HARNESS_ENGINEERING.md
- 先读上级 `AGENTS.md` 与仓库根 `/Users/skat/AGENTS.md`
- 本目录变更需与上级协议/构建脚本保持一致
- 可选 Target 可搁置；不适用形态不要硬加
