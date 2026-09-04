# M1942 Monotonic Query Expansion Contract

Date: 2026-07-13

## Question

M1939 showed that BM25 residual errors are observable and that the frozen PPLX
surface rescues 43.10% of the rows missed by both BM25 and M1914. M1940 and
M1941 nevertheless failed to turn that signal into additional heldout rescue.
Both fine-tuning routes were free to rewrite existing query impacts, and both
converged toward safer calibration rather than residual admission.

M1942 tests one structurally different hypothesis:

> Can a frozen mature sparse parent learn useful BM25-complementary query
> postings when the trainable component can only add a small number of new
> semantic dimensions?

This is the final canary in this residual family. It is not a loss or training
schedule sweep.

## Fixed Structure

- Parent: frozen M1914 Granite 30M sparse checkpoint.
- Document encoder and all document postings: frozen exactly.
- Parent query: frozen top-50 M1914 query impacts.
- Trainable component: rank-16 tokenwise residual head.
- Residual action: at most eight new non-negative query postings.
- Existing parent query dimensions cannot be removed or rewritten.
- Physical product shape: one frozen semantic posting index and one query
  encoder package containing the parent plus a small query expansion head.

The head uses straight-through top-k during training. The forward path always
has the exact deployable eight-posting budget; the backward path can assign
credit to every document-visible latent. This avoids the arbitrary-support
lock-in of hard top-k initialization.

## Training Surface

- Same 2,048 train and 512 query-disjoint heldout rows as M1939-M1941.
- Same fixed eight-candidate selection and BM25 implementation.
- Six balanced groups: Fever, HotpotQA, and NQ crossed with BM25 residual and
  protection strata.
- 1,024 updates with batch size two, covering 2,048 balanced row exposures.
- ClearML tracking is mandatory.
- No BEIR qrels and no dataset-specific thresholds.

The objective keeps the M1940/M1941 candidate KL, query support, CLEAR
residual margin, and selective PPLX terms. A DF-weighted penalty applies only
to newly added query impacts.

## Predeclared Gate

A trained checkpoint must satisfy all M1940 quality floors and additionally:

- global BM25-error rescue gain at least `+0.010`;
- equal-source macro rescue gain at least `+0.005`;
- no source rescue regression below `-0.020`;
- global harm increase at most `+0.005`;
- no source harm increase above `+0.020`;
- pairwise, positive top1, and positive MRR not below the parent;
- parent-positive top1 retention at least `0.99`;
- query support cosine at least `0.995`;
- DF-weighted query load at most `1.10x` parent;
- mean query support at most `1.20x` parent;
- document posting metrics exactly equal to the parent.

Passing this gate authorizes only native replay. It does not establish a P1
replacement or broader-corpus generalization.

## Stop Rule

If no checkpoint passes after the full balanced exposure, close the current
candidate-set residual family. Do not sweep residual weights, head rank,
posting budget, or steps. The result would mean that M1939's residual capacity
is not learnable from this local candidate supervision with a bounded
query-only posting action. The next route must change the supervision surface
or start from a native cross-corpus teacher, not tune this canary.
