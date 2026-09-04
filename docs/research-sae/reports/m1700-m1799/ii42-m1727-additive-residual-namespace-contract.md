# M1727 Frozen-Base Additive Residual Namespace Contract

## Question

M1726 shows that dense-minus-BM25 supervision is better than a matched dense
control, but jointly moving the M1600 source damages its stronger full-scale
initialization. M1727 asks the structurally different question:

> Can residual semantic keys add useful dense-minus-BM25 candidates inside the
> unused posting budget while the complete base path is frozen?

This is a representation and observability audit. It is not another training
schedule.

## Frozen Full Surface

- M1600 full 10,000/1,000 pools and 8x512 source;
- 88,992/8,988 train/validation documents;
- one frozen document key and eight frozen query probes per group;
- exact frozen BM25 top256;
- base semantic candidates and scores unchanged;
- exact dense scores used only for candidate-admission measurement;
- no qrels, dataset identity, learned gate, or threshold search.

The M1726 validation base costs `0.146086x` mean reads. Every query receives
the same fixed incremental allowance equal to the non-negative difference
between the frozen base mean and the hard `0.18x` mean cap. This aligns the
audit with the product gate: filling every individually cheap query to the cap
would exceed the mean cap because some frozen base queries already cost more
than `0.18x`. Existing base keys are never replaced.

## Two Fixed Policies

### Residual capacity oracle

For dense top256 documents absent from the frozen BM25 plus semantic candidate
set, greedily add source keys by:

```text
(10 * newly covered top100 misses + newly covered top256-tail misses)
---------------------------------------------------------------------
                         posting reads
```

The oracle may inspect dense target membership but cannot exceed the fixed
per-query incremental read budget. It tests source capacity only.

### Frozen-logit expansion

Order all non-base source keys by the frozen M1600 query logits and admit each
key if its posting fits the same fixed incremental budget. This policy uses
only inference-time information and tests whether the capacity action is
already observable from the frozen query/source geometry.

## Measurements

For base, oracle, and frozen-logit expansion report:

- unified dense O@100/O@256;
- dense-minus-BM25 recovery R@100/R@256;
- exact total and incremental posting reads;
- candidate-union ratio and max DF;
- selected key count;
- oracle-key AUC, matched-count recall, and reciprocal rank under frozen query
  logits;
- fraction of oracle metric gain captured by frozen-logit expansion.

## Gates

The capacity oracle passes only if it simultaneously achieves:

- O@100 gain `>=0.01` and O@256 gain `>=0.005`;
- R@100 gain `>=0.01` and R@256 gain `>=0.005`;
- total reads `<=0.18x` and max DF `<=0.02`.

If capacity fails, stop same-source additive residual training.

If capacity passes, a residual-only router is authorized only when either:

- frozen-logit expansion captures at least 20% of both overlap gains; or
- oracle-key AUC is at least 0.60 and matched-count recall is at least 0.10.

The follow-on router must leave the base source frozen and may only add keys
inside the incremental budget. A trained router cannot be selected by loss.

## Stop Boundary

- Do not change the base query probe count or source assignments.
- Do not sweep budgets, utility weights, score normalizations, or DF penalties.
- Do not train unless the deterministic capacity and observability gates pass.
- A directly authorized policy must itself satisfy the `0.18x` mean-read gate;
  oracle gain capture alone is insufficient.
- Exact dense reranking remains an admission upper bound, not a product scorer.
