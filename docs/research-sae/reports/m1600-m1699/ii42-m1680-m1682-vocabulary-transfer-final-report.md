# M1680-M1682 Vocabulary Transfer Final Report

Date: 2026-07-11

Decision: **retain complete Vocabulary Transfer as a valid language-interface
mechanism, but close this retrieval-training continuation. Do not run a longer
VT schedule or build an index from either M1681/M1682 branch.**

## Question

Can complete vocabulary transfer make ModernBERT a stronger learned-sparse
retrieval foundation than the simpler paper-supported MLM-head rescale?

The experiment followed three independent papers and the official SPLADE
implementation rather than a project-specific posting loss:

- [Vocabulary Transfer](https://arxiv.org/abs/2607.00004) for complete
  tokenizer, embedding, tied decoder, output-prior, MLM, and activation
  calibration transfer;
- [MLM-head rescaling](https://arxiv.org/abs/2606.18811) for the fixed
  ModernBERT `k=8` control;
- [SPLADE-v3](https://arxiv.org/abs/2403.06789) and the Naver implementation
  at commit `8dcd33a054d790e74aceda25b128c1b188c5d9c1` for MarginMSE, pooling,
  FLOPS, optimizer, and schedule semantics.

No BEIR qrels, BM25 score, project atom teacher, dataset-specific threshold,
or post-result hyperparameter grid entered training or selection.

## M1680 Interface And MLM Result

The interface audit passed every locked check:

- exact overlap-token embedding copy error: `0.0`;
- overlap/new target tokens: `4,848 / 25,674`;
- tied input/output weights preserved;
- semantic pairwise cosine Spearman: `0.320345`;
- semantic top-8 anchor overlap: `0.100098` versus mean control `0.069580`;
- ModernBERT mean head norm: `2.553124`;
- exact `k=8` mean head norm: `0.319140`.

The 500-step frozen-backbone WikiText MLM canary also learned a real language
signal:

| Metric | Step 0 | Step 500 | Change |
| --- | ---: | ---: | ---: |
| Heldout MLM loss | 8.314786 | 5.508279 | -33.8% |
| New-token loss | 13.489639 | 9.148659 | -32.2% |
| Overlap-token cosine | 1.000000 | 0.993148 | -0.006852 |
| New-token cosine | 1.000000 | 0.961379 | -0.038621 |

The paper's fixed `c=5` activation shift did not transfer to this run: token
activation fell to `0.020873`. This was not treated as a model failure because
the paper's shift is run-dependent calibration, not a universal constant.

## M1680B Disjoint Quantile APC

A single closed-form 40th-percentile shift was fit on 64 unseen WikiText rows
after all 32,064 MLM rows. It was then frozen and evaluated on disjoint
WikiText and unlabeled MS MARCO text.

| Surface | Token activation | Long-tail ratio | Finite |
| --- | ---: | ---: | --- |
| Calibration | 0.401328 | 5.427027 | yes |
| WikiText heldout | 0.408346 | 5.284211 | yes |
| MS MARCO | 0.379481 | 5.012876 | yes |

The fitted shift was `-0.3828125`. This is a valid qrels-free activation
calibration result and authorized one paired retrieval canary.

## M1681 Scaled-Ramp Failure

M1681 compressed the official 50,000-step quadratic FLOPS ramp into 167 steps
for a 500-step canary. The fixed `k=8` branch survived and reached margin MSE
`40.242638`, sign accuracy `0.733398`, and top-1 agreement `0.390625`.

The VT branch collapsed from query/document nonzeros `19,329/22,769` to
`0/0.007`. Its apparently high top-1 value was a constant-score argmax
artifact; sign accuracy was only `0.001953`. This closed the compressed-ramp
canary but did not reproduce official optimization kinetics.

## M1682 Official-Absolute Control

M1682 changed only the FLOPS ramp back to the official absolute `T=50,000`.
At step 500 the FLOPS multiplier was `1e-4` of its final value. This isolated
whether M1681 failed because of an artificially aggressive canary schedule.

| Branch | Step | Margin MSE | Sign | Top-1 | Query nnz | Document nnz |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| k8 | 0 | 787894.875000 | 0.573242 | 0.148438 | 28051.20 | 37999.42 |
| k8 | 500 | 77.045227 | 0.583984 | 0.125000 | 10134.25 | 14587.64 |
| VT | 0 | 29542100.000000 | 0.543945 | 0.156250 | 19329.50 | 22769.44 |
| VT | 500 | 62.415726 | 0.547852 | 0.164062 | 7.52 | 11583.26 |

The absolute ramp prevented all-zero collapse and VT finished with 19.0%
lower margin MSE than `k=8`. It nevertheless failed the conjunctive gate:

- VT query support retained only `0.039%` of initialization;
- VT sign accuracy was `0.036133` below `k=8`;
- VT row Spearman was only `0.035136`;
- the `k=8` branch itself lost `0.023438` top-1 agreement from initialization.

The learned VT score therefore fits margin scale through a highly asymmetric
query/document representation without recovering reliable candidate order.
Lower MSE is not a retrieval breakthrough on this surface.

## Scientific Conclusion

The experiments separate three claims:

1. Complete vocabulary transfer is implementable and preserves semantic
   topology.
2. Short qrels-free MLM plus disjoint quantile APC produces a stable language
   interface across corpora.
3. That interface did not become a reliable sparse retriever under the paired
   standard MarginMSE canary, under either compressed or official FLOPS
   kinetics.

Longer training is not authorized because M1682 did not merely stop early on
a healthy trajectory. It already reduced MSE by more than five orders of
magnitude while candidate-order correlation remained near zero and query
support collapsed. More steps would amplify an unaccepted score geometry.

This is a bounded negative result for the tested VT retrieval continuation,
not for learned-sparse retrieval. M1640/M1660 already established a positive
product-compatible mechanism: a mature normalized-vocabulary sparse root plus
an official BMP engine provides useful quality and exact single-index search.
The next route must reproduce that mature foundation at meaningful data scale
before introducing dense-root supervision.

## Artifacts

- M1680 ClearML: `fc1c114cb38244ebaf8744b19404aa72`
- M1680 MLM ClearML: `789fb8b8189c49d5adbeef330c943438`
- M1680B ClearML: `66867075884e448d9c8762e8a3521cc2`
- M1681 ClearML: `7fc4ed55548d4332959ca7359ad38551`
- M1682 ClearML: `269c85d218644a81856eb8919b956c1c`
- Remote root: `/home/huoju/leask/runs/ii42-m1680-vt-v1`

No M1680-M1682 process remains on spark-1. No failed checkpoint is promoted.
