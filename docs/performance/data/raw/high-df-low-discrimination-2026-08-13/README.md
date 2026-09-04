# High-DF experiment inventory

Date: 2026-08-13

This directory contains immutable raw evidence for the high-document-frequency
semantic posting study. None of these files defines a public query policy.

The authoritative scale-closure artifacts are:

- `arxiv-policy3-cap1000-cross-domain150.json`: native cap expansion control;
- `arxiv-seed-bmp-cross-domain150-cap128.json`: seeded-BMP control;
- `arxiv-term-budget-cross-domain150.json`: native pre-open term-budget A/B;
- `arxiv-term-budget-wide-cross-domain150.json`: 0.75%-10% work/overlap sweep;
- `arxiv-term-budget-endpoints-cross-domain150.json`: negative endpoint control
  that exposed semantic-BMP attempt/fallback duplication;
- `arxiv-term-budget-endpoints-no-bmp-cross-domain150.json`: clean 20%-100%
  endpoint sweep with semantic BMP disabled;
- `arxiv-term-budget-timing-confirm-cross-domain150.json`: repeated timing for
  the fixed 1%, 2%, and 5% points;
- `arxiv-ordered-block-budget-failed-smoke.json`: one-query negative control
  showing that query-time BMP metadata synthesis dominates block savings;
- `arxiv-accelerator-residual-budget-trec20.json`: residual-only accelerator
  budget canary and 0.5%-2% curve;
- `arxiv-accelerator-residual-budget-cross-domain150.json`: fixed 150-query
  accelerator m32 plus residual-budget qualification;
- `arxiv-accelerator-document-budget-trec20.json`: m24 document/forward-row
  work-propagation canary;
- `arxiv-accelerator-document-budget-cross-domain150.json`: fixed 150-query
  m24 document-budget rejection gate;
- `arxiv-accelerator-adaptive-threshold-trec20.json`: negative heap-factor
  canary; 0.70-0.80 opened identical clusters and scored identical documents;
- `arxiv-contextual-block-cross-domain20.json`: negative contextual block
  selector control;
- `arxiv-count-sketch-cross-domain20.json` and
  `arxiv-count-sketch-wide-cross-domain20.json`: negative CountSketch
  admission controls;
- `arxiv-document-sketch-cross-domain20.json`: document-sketch admission
  canary;
- `arxiv-quantized-forward-direct-cross-domain150.json`: fixed 150-query
  int8/int16 forward-row quality and projected-byte evidence used to select
  the v4 product format;
- `arxiv-v4-int8-native-cross-domain150.json`: native Shadow ArXiv
  exact-versus-v4-int8 A/B on the fixed 150-query cross-domain workload;
- `arxiv-v4-int8-native-trec50.json`: independent native Shadow ArXiv
  exact-versus-v4-int8 stability check on 50 TREC queries;
- `arxiv-v4-default-native-cross-domain150.json`: installed Shadow product
  default versus an explicit exact override on the fixed 150-query workload;
- `arxiv-v4-default-native-trec50.json`: independent installed-default
  qualification on 50 TREC queries;
- `arxiv-quantized-forward-cross-domain20.json` and
  `arxiv-quantized-forward-cross-domain150.json`: exact-rerank controls that
  exposed the cost of retaining duplicate forward authorities;
- `arxiv-residual-prepool-cross-domain20.json`: negative residual pre-pool
  control;
- `touche-seismic-product-45mass-selected30-q49-v3.json`: Touche 30% geometry;
- `touche-seismic-product-45mass-selected45-q49-v3.json`: Touche 45% geometry;
- `hotpot-seismic-product-45mass-selected30-q40-v3.json`: Hotpot 30% geometry;
- `touche-centered-exception-q49.json`: Touche centered-exception oracle;
- `fiqa-centered-exception-q648.json`: FIQA centered-exception oracle;
- `touche-document-block-q49.json`: Touche source-order block control;
- `hotpot-document-block-q40.json`: Hotpot source-order block control; and
- `*-retained-impact-full.json`: fixed-cap retained-impact cross-surface matrix.

Files with earlier schema versions or without the `v3` suffix are retained only
to preserve experiment history. Product decisions must use the reports in
`docs/performance/reports/` and the current bounded-execution design document,
not infer a configuration from a raw filename.

The direct quantization files predate v4 publication and deliberately read the
legacy row before computing projected compact bytes. Their latency is not a
v4 native benchmark. They establish only the quality/size gate. Use the
`arxiv-v4-int8-native-*` artifacts for forced-route A/B and the
`arxiv-v4-default-native-*` artifacts for the installed product defaults.
