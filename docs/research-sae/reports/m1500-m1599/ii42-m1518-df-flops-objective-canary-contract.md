# M1518 DF-FLOPS Objective Canary Contract

## Status

S0, the primary S1 paper-control run, and the single M1518B budget correction
are complete. Both trained branches failed the conjunctive gate: ranking and
head-share measurements remained healthy, but top DF and measured touch stayed
at 1.0. S2 is not authorized and the pooled M1510 DF-FLOPS source is closed.

## Question

Can corpus document-frequency selectivity be learned inside the existing
retrieval-trained M1510 sparse representation without sacrificing its ranking
signal?

M1516C showed that post-hoc DF15/D128/Q32 pruning improves all ArguAna quality
metrics but still touches 63.64% of the corpus. The index maximum atom DF was
already 13.95%. The failure is therefore cumulative medium-frequency fanout,
not a single high-DF atom and not absence of retrieval semantics.

## Scientific control

Use DF-FLOPS from
[Porco et al. 2025](https://arxiv.org/abs/2505.15070). For document sparse
representations `r` in a batch of size `N`:

```text
L_df_flops = sum_t ((w_t / N) * sum_i r[i, t])^2
w_t = activ(DF_t / |C|; alpha, beta)
```

The primary paper control is `alpha=0.10`, `beta=10`. DF is estimated from a
fixed qrels-free corpus sample every 100 optimizer steps. Before the first DF
refresh, all weights are one, exactly reducing to ordinary FLOPS.

Do not search alpha on BEIR. M1516B's post-hoc DF15 result is diagnostic only
and is not a training hyperparameter selection surface.

## Frozen boundaries

- Initialize from the M1510 independent SPLARE reproduction adapter.
- Keep the Gemma Scope SAE frozen.
- Train only the existing LoRA adapter/output path.
- Use MS MARCO train positives and global BM25 hard negatives.
- BM25 may mine negatives but must not enter inference scores.
- Use a disjoint MS MARCO heldout split for selection.
- Do not read NFCorpus, SciFact, FiQA, or ArguAna qrels during training,
  checkpoint selection, or hyperparameter selection.
- Preserve document K=400 and query K=40 for the first causal comparison.
  Do not confound the objective test with new pruning budgets.

The S1 data surface is pinned to
`hanhainebula/bge-multilingual-gemma2-data` revision
`ef165e19513da39a1e23bd09d78a3f1b38967e93`, config `en_msmarco`. It contains
the frozen teacher scores used by the M1510 reproduction. The deterministic
S1 subset has 10,000 train and 1,000 query-disjoint validation rows, with
eight negatives per query and seed 1518. The prepared files are only 37.4 MB;
the full dataset is not copied to spark storage.

## Stages

### S0: implementation proof

Implement and unit-test:

- ordinary FLOPS parity when every `w_t=1`;
- monotonic penalty growth with DF;
- deterministic periodic DF estimates;
- checkpoint persistence of DF statistics and objective configuration;
- finite gradients for empty and non-empty sparse rows.

Run the loss over stored M1510 representations before loading the 2B model.
The measured penalty must rank known high-DF dimensions above low-DF ones.

Implemented in `scripts/m1518_df_flops.py`, with focused coverage in
`tests/test_m1518_df_flops.py`. The ordinary-FLOPS parity, monotonic DF weight,
streaming estimate, checkpoint round-trip, and finite-gradient checks pass.
The qrels-free audit over all 3,633 saved M1510 NFCorpus document postings
also passes:

- 19,560 dimensions are active and 925 have DF at least 10%;
- the maximum observed DF is 100%;
- mean DF weight is 0.883 for DF>=10%, versus 0.008 below 10%;
- DF>20% dimensions carry 75.8% of ordinary FLOPS penalty and 86.4% of the
  conditioned penalty.

The audit is implemented in
`scripts/audit_m1518_df_flops_saved_postings.py`; its generated result is
`runs/m1518_df_flops_s0_v1/nfcorpus.df_flops_audit.json`. This confirms that
the proposed objective places gradient on the exact high-fanout dimensions
observed in M1510 rather than on an abstract synthetic failure.

### S1: paired 400-step mechanism smoke

Run two otherwise identical continuations from the same M1510 adapter and
batch order:

1. original KL plus ordinary FLOPS;
2. original KL plus DF-FLOPS (`alpha=0.10`, `beta=10`).

The smoke is not a quality result. It only asks whether DF-FLOPS changes the
intended mechanism. Log every 100 steps:

- heldout KL and pair accuracy;
- top atom DF ratio;
- head 1% posting share;
- mean active document/query dimensions;
- mean and p95 touched-document ratio on a fixed corpus/query sample;
- representation score overlap with the initialization.

Track the paired runs and checkpoint metadata in ClearML. The local JSON/log
artifacts remain the audit source if ClearML is temporarily unavailable.

Authorize S2 only if DF-FLOPS reduces both top-atom DF and mean touched ratio
relative to the paired FLOPS control, while heldout pair accuracy does not
fall by more than 2 percentage points. A falling regularizer without falling
measured DF/touch is a hard stop.

The completed primary control produced:

- exact step-0 and step-100 parity;
- final KL 0.59783 versus 0.61217 for FLOPS;
- final pair accuracy 0.90820 versus 0.88867;
- final head 1% share 0.15127 versus 0.16258;
- unchanged top atom DF and mean touch, both 1.0.

The formal decision is `stop_m1518_s1`; it does not authorize S2.

### S1B: qrels-free regularization-budget correction

The paper notes that DF-FLOPS is always no greater than FLOPS and therefore
uses a higher regularization factor. In the primary paired run, the first
DF-active batch had identical KL and query FLOPS across branches, while
ordinary document FLOPS was 681.56274 and DF-FLOPS was 233.18077. M1518B
therefore freezes the active-only scale to:

`681.562744140625 / 233.18077087402344 = 2.922894291780351`

The multiplier applies only after a DF estimator exists. Steps 0-100 must
remain exactly identical to the primary paired control. Alpha, beta, data,
adapter, batch order, optimizer, inference TopK, and the final S1 gate remain
unchanged.

If S1B does not lower top atom DF, close the pooled DF-FLOPS source. If it
lowers top DF but not touch, one final alpha correction may be proposed from
the fixed query budget and engineering touch target. It must not be selected
from BEIR results.

M1518B completed with exact step-0/100 parity. At step 400 it retained one
DF=100% dimension and another at DF=99.83%; mean touch remained 1.0. The gate
again returned `stop_m1518_s1`. Per the rule above, no alpha correction was
run. Lowering alpha cannot increase the already saturated weight on the
remaining universal dimension.

### S2: 4,000-step canary

Continue only the passing S1 configuration. Use the same fixed validation
sample and periodic DF refresh. Checkpoint every 500 steps, but select only by
a conjunctive gate:

- heldout ranking quality at least 95% of initialization;
- measured mean touched ratio at least 30% below initialization;
- candidate upper-bound proxy at least 95% of initialization;
- no checkpoint with better cost but catastrophic head ranking may win.

Training-loss minimum alone cannot select a checkpoint.

### S3: independent complete-corpus validation

Only after S2 passes, encode complete NFCorpus, SciFact, FiQA, and ArguAna
with the selected checkpoint and unchanged K400/Q40 inference. Compare native
sparse retrieval against exact M1510 baseline. Report NDCG@10, MAP@100,
Recall@100, MRR@20, CUB, max DF, head posting share, touch mean/p95, posting
count, and latency.

At least three of four rows must retain 90% of every quality metric, retain
90% CUB, and reach mean touch at most 50%. No row may fall below 75% of any
quality metric. Passing S3 authorizes a separate native-index engineering
stage; it does not promote the model by itself.

## Hard stops

- Stop if S0 cannot reproduce ordinary FLOPS exactly.
- Stop if S1 changes loss but not measured corpus DF and touch.
- Stop if S1 needs a BEIR-derived alpha or per-dataset threshold.
- Stop if two objective-weight corrections fail the same conjunctive gate.
- Stop if S2 only improves cost by deleting retrieval signal.
- Stop if S3 gains exist only on one dataset or disappear in native replay.

Do not respond to a stop by adding a post-hoc selector, rescue scorer, or BM25
fusion. A negative M1518 would mean this M1510 source cannot jointly learn the
required semantics/selectivity under the tested objective.
