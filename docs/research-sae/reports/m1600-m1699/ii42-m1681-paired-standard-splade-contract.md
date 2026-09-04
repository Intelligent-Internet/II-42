# M1681 Paired Standard SPLADE Canary Contract

## Question

Does complete Vocabulary Transfer provide a better starting point for standard
learned-sparse retrieval than the much simpler ModernBERT MLM-head rescale?

M1681 is the first retrieval-supervised experiment in this route. It does not
use P1, BM25, dense-score ensembles, project-specific atom teachers, BEIR
qrels, or a custom selector.

## Locked Reference Recipe

The implementation follows the official Naver SPLADE repository at commit
`8dcd33a054d790e74aceda25b128c1b188c5d9c1`:

- `DistilMarginMSE` on positive-versus-negative cross-encoder margins;
- `log1p(ReLU(logits))` with max pooling into one vocabulary vector;
- FLOPS regularization as squared mean activation;
- final FLOPS weights `lambda_q=0.1`, `lambda_d=0.08`;
- quadratic FLOPS ramp and AdamW with `lr=2e-5`, weight decay `0.01`;
- linear learning-rate warmup/decay.

The source is the pinned M1518 MS MARCO subset derived from
`hanhainebula/bge-multilingual-gemma2-data` with stored cross-encoder scores.
Training randomly covers one of eight negatives per row, as in standard
pairwise MarginMSE. Validation always evaluates all eight negatives.

## Paired Branches

Both branches use the identical 500-step row/negative schedule, effective
batch 16, full-model optimization, and 128 query-disjoint validation rows:

1. `rescaled_k8`: native ModernBERT tokenizer and tied head divided by eight;
2. `vt_quantile_apc`: M1680B complete vocabulary-transfer checkpoint.

The 150k-step official schedule is scaled proportionally for this mechanism
canary: warmup `20/500` and FLOPS ramp `167/500`. This is not a benchmark
reproduction. It only decides whether a larger run is justified.

## Metrics

At steps 0, 100, 250, and 500 report:

- full-eight-negative teacher margin MSE;
- teacher-margin sign accuracy and teacher top-1 agreement;
- row score Spearman;
- query/document mean nonzeros, FLOPS, and maximum sampled DF;
- finite gradients and score statistics.

## Gate

Each branch passes only if, relative to its own initialization:

- margin MSE improves by at least `5%`;
- sign accuracy and teacher top-1 do not fall by more than `0.005`;
- query/document nonzeros, FLOPS, and maximum DF are each at most `1.10x`;
- all metrics remain finite.

Authorize a complete FiQA/native-index retrieval smoke only if the VT branch
passes and its final margin MSE is at least `5%` lower than the final `k=8`
control, with sign accuracy and top-1 no more than `0.005` worse.

## Stop Rules

- Do not search rescale factors, APC targets, loss weights, or negatives.
- Do not substitute a custom ensemble teacher if MarginMSE fails.
- Do not infer retrieval quality from training loss alone.
- Stop before index construction if VT cannot beat the simple rescale control.
- A pass authorizes FiQA exact retrieval only, not shared15 or promotion.
