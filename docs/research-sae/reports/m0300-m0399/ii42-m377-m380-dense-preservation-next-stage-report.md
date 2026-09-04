# II-42 M377-M381 Dense-Preservation Next Stage Report

## Scope

This sequence continued the dense-only line. It does not use BM25, dataset id,
target qrels for training, or benchmark-specific hyperparameter tuning.

The question was whether the remaining gap is caused by admission, scorer
routing, neural residual modeling, or dense information lost by the posting
representation.

## Runs

| Run | Output | Scope |
| --- | --- | --- |
| M377 | `/tmp/ii42-m377-query-corpus-router/m377_query_corpus_router.json` | Query/corpus router over fixed dense-only scorers |
| M378 | `/tmp/ii42-m378-anchored-residual-ranker/m378_anchored_residual_ranker.json` | Sparse-pca anchored neural residual ranker |
| M379 | `/tmp/ii42-m379-tail-sketch-dense-preservation-broad4-fullq/m379_tail_sketch_dense_preservation.json` | Random tail sketch, broad4 |
| M380 | `/tmp/ii42-m380-tail-pca-sketch-dense-preservation-beir15/m380_tail_pca_sketch_dense_preservation.json` | Corpus PCA tail sketch, BEIR15 |
| M381 | `/tmp/ii42-m381-tail-pca-sketch-size-sweep-beir15/m381_tail_pca_sketch_size_sweep.json` | PCA tail sketch size sweep, BEIR15 |
| M383 | `/tmp/ii42-m383-joint-pca-tail-sketch-beir15/m383_joint_pca_tail_sketch.json` | Joint doc/query PCA tail sketch, BEIR15 |
| M384 | `/tmp/ii42-m384-query-pca-heldout-beir15/m384_query_pca_heldout_beir15.json` | Query-PCA with 50/50 query-heldout split |
| M386 | `/tmp/ii42-m386-joint-pca-heldout-beir15/m386_joint_pca_heldout_beir15.json` | Joint-PCA with 50/50 query-heldout split |
| M389 | `/tmp/ii42-m389-joint-pca-heldout-seed1379-beir15/m389_joint_pca_heldout_seed1379.json` and `/tmp/ii42-m389-joint-pca-heldout-seed2379-beir15/m389_joint_pca_heldout_seed2379.json` | Extra query-heldout robustness seeds |
| M390 | `/tmp/ii42-m390-joint-pca-heldout-int8-beir15/m390_joint_pca_heldout_int8.json` | Doc-sketch quantization |
| M391 | `/tmp/ii42-m391-joint-pca-budget005-beir15/m391_joint_pca_budget005.json` and `/tmp/ii42-m391-joint-pca-budget008-beir15/m391_joint_pca_budget008.json` | Touch-budget sweep |

## M377 Router Result

M377 trained a global router from observable query/corpus/candidate-pool
features. It selected among sparse/raw, sparse/pca, neural/raw, and neural/pca.
The router never saw dataset id and used dense-teacher proxy labels, not qrels.

On the four stress heldouts:

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG@10 | Dense O@10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_exact | 0.6972 | 0.8170 | 0.7055 | 0.4886 | 0.7055 | 1.0000 |
| m377_sparse_pca | 0.6917 | 0.8039 | 0.6950 | 0.4795 | 0.7062 | 0.7955 |
| m377_router | 0.6907 | 0.8035 | 0.6951 | 0.4792 | 0.7062 | 0.7958 |

Verdict: routing is not the blocker. It collapses to sparse_pca with only
noise-level changes.

## M378 Anchored Residual Result

M378 kept sparse_pca as the anchor and allowed a neural residual only when
source dense-teacher alpha selection accepted it.

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG@10 | Dense O@10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_exact | 0.6972 | 0.8170 | 0.7055 | 0.4886 | 0.7055 | 1.0000 |
| m378_sparse_pca | 0.6917 | 0.8039 | 0.6950 | 0.4795 | 0.7062 | 0.7955 |
| m378_residual_select | 0.6916 | 0.8030 | 0.6946 | 0.4796 | 0.7065 | 0.7955 |
| m378_residual_rerank | 0.6916 | 0.8030 | 0.6946 | 0.4796 | 0.7062 | 0.7955 |

Verdict: neural residual correction is not reliable enough. It helped
trec-covid slightly but hurt fiqa, and macro regressed.

## M379/M380 Dense Tail Result

M379 first exposed the correct decomposition. Reranking by only tail-tail was
insufficient because it missed active-doc/full-query cross terms. The corrected
full-query tail form is:

`score ~= doc_active dot query_full + doc_tail_sketch dot query_full_sketch`

This preserves posting admission while restoring dense tail information for the
touched set.

On BEIR15 full, corpus-PCA tail sketch is the first strong positive result in
this sequence:

| Source | Sketch | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG@10 | Dense O@10 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_exact | 768 | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 0.7759 | 1.0000 | 1.0000 |
| sparse_pca | 0 | 0.8501 | 0.8622 | 0.7673 | 0.6856 | 0.7761 | 0.8193 | 0.1000 |
| fullq_tail_sketch_8 | 8 | 0.8523 | 0.8674 | 0.7714 | 0.6895 | 0.7761 | 0.8761 | 0.1000 |
| fullq_tail_sketch_32 | 32 | 0.8535 | 0.8671 | 0.7723 | 0.6904 | 0.7761 | 0.8823 | 0.1000 |
| fullq_tail_sketch_128 | 128 | 0.8531 | 0.8692 | 0.7730 | 0.6911 | 0.7761 | 0.9010 | 0.1000 |
| fullq_tail_sketch_256 | 256 | 0.8532 | 0.8718 | 0.7754 | 0.6944 | 0.7761 | 0.9256 | 0.1000 |
| fullq_tail_exact | 640 | 0.8536 | 0.8722 | 0.7761 | 0.6946 | 0.7761 | 0.9968 | 0.1000 |

Delta versus sparse_pca NDCG@10:

| Variant | Delta |
| --- | ---: |
| fullq_tail_sketch_8 | +0.0041 |
| fullq_tail_sketch_16 | +0.0048 |
| fullq_tail_sketch_32 | +0.0050 |
| fullq_tail_sketch_64 | +0.0042 |
| fullq_tail_sketch_128 | +0.0057 |
| fullq_tail_sketch_256 | +0.0081 |
| fullq_tail_exact | +0.0088 |

## M381 Size Sweep

M381 swept the practical sketch-size region on BEIR15 full.

| Sketch | NDCG@10 | Delta vs sparse | Gap recovered | MRR@20 | MAP@100 | Dense O@10 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 0.7715 | +0.0042 | 47.8% | 0.8671 | 0.6898 | 0.8892 |
| 96 | 0.7721 | +0.0048 | 54.1% | 0.8684 | 0.6909 | 0.8960 |
| 128 | 0.7730 | +0.0057 | 64.2% | 0.8692 | 0.6911 | 0.9010 |
| 160 | 0.7735 | +0.0061 | 69.7% | 0.8675 | 0.6912 | 0.9072 |
| 192 | 0.7739 | +0.0066 | 74.8% | 0.8683 | 0.6916 | 0.9128 |
| 224 | 0.7742 | +0.0069 | 78.2% | 0.8697 | 0.6926 | 0.9185 |
| 256 | 0.7754 | +0.0081 | 91.3% | 0.8718 | 0.6944 | 0.9256 |

Practical elbow: 128 is already a meaningful lightweight variant; 192 is the
balanced target; 256 is the near-dense target.

## M383-M386 Projection Stress Test

M383 tested query-aware projections. A purely transductive query-PCA can reach
the upper bound at low dimensions because the local benchmark query subspace is
small. That result is useful as a diagnostic but too optimistic as an
implementation target. M384 therefore repeated query-PCA with a 50/50
projection-query versus eval-query split. M386 repeated the split with joint
doc/query PCA.

| Run | Sparse | 64 | 96 | 128 | 160 | 192 | 224 | 256 | Upper |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| doc_pca_allq | 0.7673 | 0.7715 | 0.7721 | 0.7730 | 0.7735 | 0.7739 | 0.7742 | 0.7754 | 0.7761 |
| query_pca_heldout | 0.7690 | 0.7727 | 0.7743 | 0.7739 | 0.7731 | 0.7733 | 0.7741 | 0.7736 | 0.7752 |
| joint_pca_allq | 0.7673 | 0.7746 | 0.7744 | 0.7748 | 0.7751 | 0.7754 | 0.7754 | 0.7757 | 0.7761 |
| joint_pca_heldout | 0.7690 | 0.7723 | 0.7714 | 0.7737 | 0.7729 | 0.7739 | 0.7744 | 0.7752 | 0.7752 |

Current best clean recommendation: joint PCA, 256 sketch dims. It reaches
0.7752 NDCG@10 on the query-heldout split, matching the upper bound on that
split and slightly exceeding the corresponding dense_exact macro of 0.7748.

Teacher-calibrated dimension selection was also tested, but the dense-teacher
calibration scores saturated on the small calibration query subspaces and did
not reliably choose the best heldout dimension. Do not promote that selector
yet.

## M389 Robustness

Additional 50/50 query-heldout seeds confirmed that joint-PCA 256 is not a
single-split artifact.

| Seed | Sparse | Sketch256 | Dense | Upper | Delta vs sparse |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 379 | 0.7690 | 0.7752 | 0.7748 | 0.7752 | +0.0062 |
| 1379 | 0.7715 | 0.7816 | 0.7812 | 0.7811 | +0.0101 |
| 2379 | 0.7536 | 0.7606 | 0.7613 | 0.7613 | +0.0070 |

## M390 Quantization

Doc-side sketch quantization was tested with joint-PCA 256 on the query-heldout
split. Query sketches remained float at query time.

| Quantization | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| none | 0.8551 | 0.8680 | 0.7752 | 0.6933 | 0.9295 | 0.1000 |
| fp16 | 0.8551 | 0.8680 | 0.7752 | 0.6933 | 0.9295 | 0.1000 |
| int8 | 0.8551 | 0.8680 | 0.7751 | 0.6934 | 0.9295 | 0.1000 |
| int4 | 0.8548 | 0.8687 | 0.7749 | 0.6934 | 0.9282 | 0.1000 |

Int8 is effectively lossless under this test. Int4 is surprisingly close but
should be treated as an aggressive storage option.

## M391 Budget Sweep

Joint-PCA 256 was tested at lower touched ratios.

| Touch target | Sparse NDCG@10 | Sketch NDCG@10 | Upper NDCG@10 | Sketch R@100 |
| ---: | ---: | ---: | ---: | ---: |
| 0.05 | 0.7690 | 0.7756 | 0.7752 | 0.8493 |
| 0.08 | 0.7690 | 0.7756 | 0.7752 | 0.8533 |
| 0.10 | 0.7690 | 0.7752 | 0.7752 | 0.8551 |

Top-10 ranking quality survives down to 5% touch in this sampled face, but
R@100 drops. This means low-touch variants are viable for ranking latency, but
recall-sensitive settings should stay at 8-10% unless admission is improved.

## Interpretation

The highest-value direction is now clear: preserve dense tail information in
the posting representation. Admission is already strong enough at 10% touch;
the upper bound is essentially dense. Scorer routing and pointwise neural
residuals are weak because they try to infer missing dense information from
posting statistics after it has already been discarded.

Corpus PCA tail sketch is clean under the current constraint: it adapts to the
corpus distribution, not to dataset labels or qrels. It also answers the core
SAE concern directly: the model can approach dense when the representation
keeps enough dense residual capacity.

## Next Step

Promote this line into an implementation-oriented M392:

- Store active signed-coordinate postings plus a compact per-doc joint-PCA tail
  sketch.
- At query time, compute active coordinates and the query full/sketch vector.
- Use postings for admission, then rerank touched docs with
  `doc_active dot query_full + doc_tail_sketch dot query_full_sketch`.
- Use 192 as the first storage/latency tradeoff target and 256 as the
  near-dense quality target. For the most conservative clean target, use
  joint-PCA 256.
- Store doc sketches as int8 by default; keep fp16 as the conservative
  baseline and int4 as an aggressive optional mode.
- Treat 5% touch as an NDCG/latency mode and 8-10% touch as the safer
  recall-preserving mode.
- Keep the evaluation BM25-free and qrels-free except final metrics.
