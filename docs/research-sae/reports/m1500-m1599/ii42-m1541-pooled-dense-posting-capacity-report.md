# M1541 Pooled-Dense Posting Capacity Audit

## Decision

**Stop the raw signed-coordinate source as the bounded admission mechanism.**

M1541 is a useful structural negative result, not a failed training run. It
shows that pooled BGE coordinates plus an INT8 tail sketch have enough scoring
capacity, but the historical quality depends on reading essentially the whole
coordinate index before selecting the final 8% of documents.

Commit under test: `ee94f0de`.

ClearML task: `c40d644ef1624085af06a604911eb43a`.

## Experiment

- Frozen `BAAI/bge-base-en-v1.5` encoder.
- Exact M1520 NFCorpus, SciFact, and FiQA canaries.
- Document-only PCA rotation.
- 128 signed active document/query coordinates.
- 256-dimensional INT8 document tail sketch.
- Qrels used only after rankings were produced.
- Honest 8% and 15% posting-edge traversal surfaces.
- Historical M392 shape retained as a capacity ceiling: prefix 256 from every
  query coordinate, then keep 8% by accumulated sparse score.

The key correction from old M392 reporting is that M1541 records both raw
posting traversal and the later rerank budget. It never labels an 8% rerank
cap as an 8% index touch when the source first scanned a much larger union.

## Primary 15% Result

| Dataset | Source | NDCG@10 | Recall@100 | CUB | Dense O@100 | Posting reads | Raw union |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | exact BGE | 0.458739 | 0.386586 | 0.812651 | 1.000000 | 1.0000x | 1.0000 |
| nfcorpus | 15% dense upper | 0.132693 | 0.089819 | 0.176932 | 0.283600 | 0.1503x | 0.1503 |
| nfcorpus | 15% tail256 INT8 | 0.132080 | 0.088298 | 0.176932 | 0.283600 | 0.1503x | 0.1503 |
| scifact | exact BGE | 0.848783 | 0.980000 | 1.000000 | 1.000000 | 1.0000x | 1.0000 |
| scifact | 15% dense upper | 0.210035 | 0.207500 | 0.217500 | 0.258900 | 0.1500x | 0.1500 |
| scifact | 15% tail256 INT8 | 0.210035 | 0.207500 | 0.217500 | 0.258900 | 0.1500x | 0.1500 |
| fiqa | exact BGE | 0.694897 | 0.934806 | 0.996667 | 1.000000 | 1.0000x | 1.0000 |
| fiqa | 15% dense upper | 0.158925 | 0.183996 | 0.183996 | 0.302900 | 0.1500x | 0.1500 |
| fiqa | 15% tail256 INT8 | 0.158579 | 0.183996 | 0.183996 | 0.302900 | 0.1500x | 0.1500 |

Gate result: `0/3`; scale was not authorized.

The exact-dense upper and tail256 rows are nearly identical. The loss occurs
before tail scoring: bounded max-impact posting traversal admits the wrong
documents. Increasing sketch size or reconstruction training cannot recover
documents that never enter the candidate set.

## Historical Capacity Ceiling

| Dataset | Source | NDCG@10 | Recall@100 | Dense O@100 | Posting reads | Raw union | Rerank |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | historical tail256 | 0.461830 | 0.385605 | 0.906100 | 13.4031x | 1.0000 | 0.0805 |
| scifact | historical tail256 | 0.849605 | 0.980000 | 0.923300 | 14.0204x | 1.0000 | 0.0800 |
| fiqa | historical tail256 | 0.690943 | 0.933615 | 0.934600 | 14.1208x | 1.0000 | 0.0800 |
| macro | exact BGE | 0.667473 | 0.767131 | 1.000000 | 1.0000x | 1.0000 | 1.0000 |
| macro | historical tail256 | 0.667459 | 0.766407 | 0.921333 | 13.8481x | 1.0000 | 0.0802 |

This is strong evidence that the signed-coordinate representation and compact
tail score are sufficient after a good candidate set exists. It is equally
strong evidence that the old M392 shape is not a deployable inverted-index
retrieval algorithm: it reads about 13.8 posting entries per corpus document,
unions 100% of the corpus, and only then hides the cost behind an 8% rerank.

## Mechanism Diagnosis

The bounded merge prioritizes the largest individual `q_i * d_i` edge. Dense
neighbors are instead defined by the sum of many moderate coordinate
contributions. At 15% reads, almost every consumed edge belongs to a different
document, so the method never accumulates enough evidence for the true dense
neighbors. This explains all three observations:

1. 15% sparse ranking is extremely weak;
2. exact-dense reranking cannot repair it because admission already failed;
3. full-union sparse accumulation selects an excellent 8% set.

The bottleneck is therefore the candidate access structure, not BGE, the PCA
basis, INT8 quantization, the tail sketch, or training depth.

## Next Authorized Probe

M1542 should replace independent coordinate-edge truncation with a qrels-free
corpus-routing posting source:

- fit a document-only spherical IVF/codebook over pooled BGE vectors;
- publish one or a small fixed number of route atoms per document;
- let the query multi-probe route atoms until the exact raw union reaches 8%
  and 15%;
- reuse the frozen M1541 tail256 scorer over that union;
- report route-list reads, raw union, dense overlap, and retrieval quality;
- compare single-assignment and one predeclared multi-assignment route only;
- do not train an output head or use qrels unless the 15% route passes the
  admission gate.

This is structurally different from M1520: M1520 clustered token states into
semantic atoms. M1542 tests corpus-level routing of the already validated
pooled dense geometry. If document-only routing cannot recover at least 90%
dense O@100 at 15% union, the pooled-BGE posting route should stop rather than
enter another loss search.

## Artifacts

- `runs/m1541a_pooled_dense_posting_capacity_v1/summary.json`
- `runs/m1541a_pooled_dense_posting_capacity_v1/summary.md`

