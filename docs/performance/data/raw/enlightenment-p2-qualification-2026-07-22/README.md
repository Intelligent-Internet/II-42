# Enlightenment P2 Qualification Evidence

This directory contains the raw evidence cited by the 2026-07-22
Enlightenment P2 production qualification report.

This is an immutable historical evidence archive. Several files exercise
catalog, ABI, and binary transitions that the current package intentionally
does not install or support. They must not be used as current release gates.

| File | Surface |
| --- | --- |
| `lifecycle.json` | Final package, actual P2 model, 42-gate lifecycle suite |
| `matched-arxiv75k.json` | Matched BM25, VectorChord, exact-vector, and P2 query matrix |
| `p22-exact-query-optimization.json` | Exact P2.2 scorer parity, latency, and memory gate |
| `arxiv75k-p2-build.log` | Concurrent 75k arXiv P2 build timing |
| `arxiv75k-p2-soak.json` | 300-query P2 latency and memory soak |
| `concurrent-ddl.json` | Concurrent writer, REINDEX, rewrite, and drop gates |
| `pubmed1987-build.log` | Cross-domain PubMed BM25/P2 build and status |
| `pubmed1987-benchmark.json` | PubMed query, concurrency, and identity proxy |
| `qos-threads0.json` | Real-model ONNX default thread behavior |
| `qos-threads2.json` | Real-model ONNX two-thread cap behavior |
| `qos-threads4.json` | Real-model ONNX four-thread cap behavior |
| `p22-package-bound-maturity-summary.json` | Local staged P2.2 package identity and 18-step maturity result |
| `p22-replication-lifecycle.json` | Local P2.2 primary/standby ranked lifecycle parity |
| `p22-native-scifact-qrels-summary.json` | Full-corpus native PG18 SciFact held-out relevance and cost gate |
| `p22-native-arguana-qrels-summary.json` | Full-corpus native PG18 Arguana held-out relevance and cost gate |
| `scidocs-official-load.json` | Official 25,657-document SciDocs loader provenance and row-count gate |
| `scidocs-official-native-qrels.json` | Full native PG18 SciDocs query rows, metrics, index metadata, and latency |
| `scidocs-official-native-qrels.md` | Rendered SciDocs BM25/P2.2 comparison matrix |
| `scidocs-official-native-qrels.log` | ARM64 container build and complete 1,000-query SciDocs campaign log |
| `trec-covid-official-load.json` | Official TREC-COVID corpus SHA-256 and strict 171,331-row loader gate |
| `trec-covid-official-build.json` | Native BM25/P2.2 full-corpus build times |
| `trec-covid-official-native-qrels.json` | Complete 50-query strict native TREC-COVID result and index metadata |
| `trec-covid-official-native-qrels.md` | Rendered TREC-COVID BM25/P2.2 comparison |
| `trec-covid-official-native-qrels.log` | Corrected MAP@100 strict evaluator progress and summary |
| `trec-covid-12-queries.json` | Fixed real-query source for exact scorer A/B |
| `trec-covid-baseline.json` | Full-sort exact baseline on the 171,331-document volume |
| `trec-covid-bounded.json` | First bounded-heap exact candidate run; rejected on latency |
| `trec-covid-bounded-warmed.json` | Warm bounded-heap rerun confirming the latency regression |
| `p22-native-heldout-qrels-matrix.json` | Normalized native held-out matrix with per-dataset and macro metrics |
| `p22-native-heldout-qrels-matrix.md` | Rendered four-row held-out BM25/P2.2 matrix |
| `p22-contract-v2-legacy-parity.json` | Legacy arXiv generation identity and score-tolerance parity after candidate deployment |
| `p22-contract-v2-legacy-model-drift.json` | Legacy generation model-drift fail-closed and exact restoration gate |
| `p22-contract-v2-v2-lifecycle-soak.json` | Enlightenment ABI-v2/EATMH003 unified CRUD, full-text, restart, and soak result |
| `p22-contract-v2-offline-rollback-restore.json` | Offline baseline rollback and P2.2 candidate restoration evidence |
| `p22-contract-v2-arxiv-performance-round1.json` | First post-deployment arXiv 75k candidate latency round |
| `p22-contract-v2-arxiv-performance-round2.json` | Second post-deployment arXiv 75k candidate latency round and samples |
| `p22-contract-v2-exact-performance-ab.json` | Same-window unoptimized/candidate exact latency and 1,200-row parity A/B |
| `exact-aba-baseline-b.json` | Final exact optimization unoptimized 12-query, 120-sample baseline |
| `exact-aba-bulk-slice-v1.json` | final-v2 exact bulk-slice candidate C1 |
| `exact-aba-bulk-slice-v1-c2.json` | Independent final-v2 candidate C2 |
| `exact-aba-final-v2-c3.json` | Installed final-v2 package C3 |
| `rollback-final-v1-after-v2.json` | Controlled final-v2 to final-v1 rollback identity and score parity |
| `reinstall-final-v2.json` | Immediate post-restart observation retained as preload-contaminated evidence |
| `reinstall-final-v2-stable-c4.json` | Stable post-rollback/reinstall final-v2 exact performance gate |
| `p22-final-v2-qualification-summary.json` | Combined exact, package, maturity, lifecycle, rollback, and final verdict |
| `package-binding-final-v2.json` | Installed-versus-staged final-v2 package fingerprint |
| `maturity-v4-final-v2.json` | Final-v2 17-step package-bound maturity result |
| `maturity-v4-final-v2.model-lifecycle.json` | Final-v2 48/48 model lifecycle |
| `maturity-v4-final-v2.medium-lifecycle.json` | Final-v2 50k-document mutable lifecycle |
| `mutable-lifecycle-bulk-slice-v1.json` | Pre-package exact candidate 46/46 lifecycle |
| `p22-final-v2-BUILD-INFO.txt` | Final-v2 build provenance |
| `p22-final-v2-SHA256SUMS` | Final-v2 package manifest |
| `p22-final-v3-BUILD-INFO.txt` | Final-v3 0.2.0 build provenance |
| `p22-final-v3-SHA256SUMS` | Final-v3 package manifest |
| `p22-final-v3-preupgrade-catalog.json` | Live 0.1.2 function/index catalog before versioned upgrade |
| `p22-final-v3-postupgrade-catalog.json` | Live 0.2.0 catalog parity and preserved index identities |
| `p22-final-v3-preupgrade-arxiv.json` | Fixed 1,200-row arXiv ranking before catalog upgrade |
| `p22-final-v3-postupgrade-arxiv.json` | Immediate post-upgrade ranking parity |
| `p22-final-v3-host-reboot-crash-recovery.txt` | External abrupt-host-reboot WAL and index recovery evidence |
| `ii42-p22-final-v3-binary-rollback-v2.txt` | Binary-only final-v2 rollback with catalog retained at 0.2.0 |
| `ii42-p22-final-v3-binary-rollback-v2-arxiv.json` | Exact ranking parity under rollback binary |
| `ii42-p22-final-v3-binary-restore.txt` | Restored final-v3 SHA, catalog/API, and index gates |
| `ii42-p22-final-v3-binary-restore-arxiv.json` | Exact final-v3 restore ranking and latency check |
| `ii42-p22-final-v3-postupgrade-steady-arxiv.json` | First 120-sample final-v3 post-upgrade exact round |
| `ii42-p22-final-v3-postupgrade-steady-r2-arxiv.json` | Independent post-upgrade exact round under host load |
| `scifact-p22-budget075-model-single-variable.txt` | Audited one-variable model diff and artifact hashes |
| `scifact-p22-baseline-v3-postreboot-qrels.json` | Paired post-reboot exact P2 SciFact baseline |
| `scifact-p22-budget075-v3-postreboot-qrels.json` | Budget-0.75 native SciFact canary |
| `scifact-p22-budget075-v3-canary-gate.json` | Rejected quality/storage/latency Pareto gate |
| `scifact-p22-budget075-v3-canary-gate.md` | Rendered lossy canary decision |
| `maturity-v5-final-v3.json` | Final-v3 18-step package-bound maturity result |
| `maturity-v5-final-v3.model-lifecycle.json` | Final-v3 production model 48/48 lifecycle |
| `maturity-v5-final-v3.concurrent-ddl.json` | Final-v3 concurrent writer/DDL 8/8 gate |
| `maturity-v5-final-v3.onnx-soak.json` | Healthy and expected-failure 1,000-iteration memory soaks |
| `maturity-v5-final-v3.medium-lifecycle.json` | Final-v3 50k build, unified maintain, REINDEX, and drop |
| `maturity-v5-final-v3.log` | Ordered final-v3 maturity execution log |
| `maturity-v5-final-v3-harness-incomplete.json` | Retained pre-product failure from incomplete qualification metadata |
| `maturity-v5-final-v3-harness-incomplete.log` | Harness-only failure log before metadata repair |
| `ii42-p22-final-v3-live-postflight.txt` | Final live service, binary, catalog, runtime, index, and leftover-process audit |

The query matrices use deterministic title/self queries and have no qrels.
They are operational correctness and performance evidence, not relevance
benchmarks. The stored 256-dimensional vectors are not proven to originate
from the same root as the P2 Granite sparse model, so cross-model overlap must
not be interpreted as dense-equivalence evidence.

The earlier `p22-*` files are local candidate-package evidence. The
`p22-contract-v2-*` files record the first target-host candidate. The
`exact-aba-*`, `maturity-v4-final-v2*`, rollback/reinstall, and final summary
files bind the closing result to the installed final-v2 binary. The legacy
arXiv generation remains ABI-v1 until an explicit model migration and REINDEX;
isolated ABI-v2 generations provide the mutable and full-text qualification.

The retained final-v3 harness failure happened before any lifecycle step: the
remote source snapshot lacked workflow and documentation files required by the
static convergence inventory. After synchronizing only those metadata inputs
and assigning the dedicated snapshot to the postgres qualification user, the
inventory passed independently and the canonical 18-step maturity run passed.
It is not a package or runtime failure.
