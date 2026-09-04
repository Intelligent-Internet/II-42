# M1530A Retrieval-Supervised URSI Head

Decision: **stop the independent 32K-centroid head parameterization after
M1530A**. Retain the paired-retrieval objective and bounded posting shape, but
do not scale this full 25.2M-parameter head to the 10K/1K surface.

## Question

M1520C showed that the qrels-free 32K vocabulary had acceptable posting cost
but did not preserve dense-recoverable query-document alignment. M1530A asked
whether paired retrieval supervision could repair that alignment while the
BGE backbone, posting budgets, and background filter remained fixed.

The experiment deliberately excluded BM25, qrels, routing, reconstruction,
FLOPS, backbone unfreezing, and hyperparameter search. It trained one shared
query/document concept projection with hard token TopK=8, query K=24,
document K=96, non-negative impacts, and one identifiable score-calibration
scalar.

## Locked Surface

- Frozen backbone: `BAAI/bge-base-en-v1.5`.
- Initialization: the M1520C 32K centroids, SHA-256
  `e699d795a78c8fe7b117fac1027684d80202f730528f93424306b7de77de550d`.
- Teacher data: 256 train and 128 disjoint validation rows from the pinned
  M1518 MS MARCO artifact. Each row contains one positive, eight hard
  negatives, and frozen cross-encoder scores.
- Objective: candidate-set KL between frozen teacher scores and semantic
  posting dot products.
- Trainable parameters: 25,198,593.
- Runtime: NVIDIA PyTorch 26.03, CUDA 13.2, PyTorch 2.11 nightly,
  Transformers 5.3.0, ClearML 2.1.10 on `spark-1`.
- Trainer SHA-256:
  `7a04e86c6d9200619703516a6060092354d99a7797eba348429865d8b7f73a54`.

The 200-step canary and predeclared 800-step depth extension are tracked as
ClearML tasks `0339389ad08d4bacbe17fb5148aeee7e` and
`58348d2506784df3a33008170d68b8da`.

## Result

| Step | KL | Teacher top1 | Pair order | Positive top1 | Touch mean | Touch p95 | Max DF | Active concepts |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 1.508126 | 0.476562 | 0.837573 | 0.460938 | 0.078369 | 0.151519 | 0.100694 | 21,315 |
| 100 | 1.484059 | 0.476562 | **0.848337** | 0.468750 | 0.079827 | 0.155946 | 0.099826 | 21,377 |
| 200 | 1.461921 | 0.484375 | 0.845401 | 0.484375 | 0.080946 | 0.150434 | 0.101562 | 21,284 |
| 250 | **1.441690** | 0.500000 | 0.835616 | **0.515625** | 0.083062 | 0.156250 | 0.101562 | 21,203 |
| 300 | 1.507292 | **0.507812** | 0.834638 | 0.492188 | 0.084235 | 0.158854 | 0.101562 | 21,146 |
| 500 | 1.485375 | 0.437500 | 0.831703 | 0.453125 | 0.092428 | 0.169531 | 0.105035 | 20,939 |
| 800 | 1.497407 | 0.398438 | 0.831703 | 0.437500 | 0.099270 | 0.182292 | 0.106771 | 20,709 |

The lowest-KL checkpoint was step 250:

- KL improved by 4.41%, below the required 10%;
- teacher top1 improved by 0.023438, below the required 0.05;
- pair-order agreement decreased by 0.001957;
- touch increased by 5.99% relative but remained far below its 30% gate;
- max DF increased by only 0.000868 absolute;
- active vocabulary decreased by 0.53% relative.

No checkpoint passed either individual alignment threshold, and no checkpoint
passed the conjunctive promotion gate. The best top1 checkpoint at step 300
did not preserve its KL improvement. Validation quality then degraded through
step 800 while the posting-integrity measurements remained healthy.

## Interpretation

M1530A provides a weak but real positive signal: paired retrieval supervision
can move winner selection and KL without collapsing posting cost. The failure
is not caused by touch, universal concepts, negative impacts, numerical
instability, insufficient optimizer steps, or a query/document formatting
mismatch.

The failed parameterization gives every one of 32K concepts an independently
trainable 768-dimensional vector. Hard TopK sends gradients only through the
currently selected concepts, so 256 training rows provide very sparse
coverage for 25.2M free parameters. The step-250 improvement followed by
validation regression is consistent with poor shared statistical strength,
not with a need for more epochs. The deterministic 800-step replay rules out
run noise and exhausts the predeclared depth extension.

This does not close retrieval-supervised URSI. It closes the assumption that
small paired data can safely fine-tune all centroid coordinates independently.

## Next Probe

The next justified experiment is a new parameterization, not a threshold or
loss grid:

1. Freeze all 32K M1520 centroids and the BGE backbone.
2. Insert one shared low-rank residual transform in the 768-dimensional token
   space before the frozen centroids.
3. Keep the exact M1530A teacher, KL objective, budgets, validation rows, and
   integrity gates.
4. Compare rank 8/32 only after a rank-8 capacity smoke shows a valid gradient
   and non-flat ordering signal; do not run a broad grid.
5. Stop the source if shared low-rank adaptation cannot beat M1530A's step-250
   ordering/KL trade-off without violating the same posting gates.

This reduces trainable geometry parameters by roughly three orders of
magnitude and makes every example update a shared basis. Only if that control
passes should the route scale to 10K/1K or consider a final-layer LoRA.

## Artifacts

Bounded JSON, JSONL, environment, and logs are mirrored under
`runs/m1530a_retrieval_ursi_head_v1/`. The 49 MB initial and diagnostic
checkpoints remain on `spark-1`; no failed checkpoint is promoted or committed.
