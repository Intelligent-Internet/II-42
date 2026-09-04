# M1943-M1945 Residual Observability Closure

Date: 2026-07-13

## Executive Decision

The retrieval-conditioned full-tail residual-selector family is closed. It
does not proceed to native replay or neural training.

The investigation found a real low-DF target and a real target-versus-harm
signal, but it also proved that the signal cannot rank targets through the
complete query-visible action space. The positive M1944 sampled AUC was not a
deployable breakthrough.

## Evidence Chain

### M1943: target existence and fixed proposal failure

M1943 established that residual target latents are not universal expansion
terms:

- 1,894 distinct targets across 264 FiQA/SciFact target-bearing queries;
- normalized target entropy `0.9950`;
- target document-DF median near `0.3%` and p95 near `2.3%`;
- full ranks 96-1000 expose `100%` of targets.

However, the fixed qrels-free top-64 proposal recovered only `1.81%` of target
assignments and hit a target on `10.61%` of queries. Median positive proposal
ranks were 884 on FiQA and 1,253 on SciFact. A fixed compact proposal was
therefore rejected.

### M1944: sampled separability

M1944 widened the source to the complete ranks 96-1000 plus BM25 and semantic
top-32 context. On FiQA, ArguAna, NFCorpus and SciFact:

- source target recall was `1.0` on every row;
- source dimensions averaged 7,381-10,572 per query;
- source posting reads stayed under the predeclared 150k p95 floor;
- target-versus-harm LODO AUC was `0.99698`;
- target-versus-rest LODO AUC was `0.98860`.

This passed the predeclared gate for one selector canary. The important scope
limit is that each query used all target/harm labels but only 32 hard neutral
dimensions. It tested sampled class separation, not complete-source ranking.

### M1945: complete-source counterexample

M1945 held out each corpus, fit the two fixed M1944 logistic models on the
other corpora, and ranked every source dimension with the predeclared score:

`sqrt(P(target_vs_rest) * P(target_vs_harm))`

No classifier, threshold, feature or budget was swept.

| Method | K | Target Recall | Any-Target Query | Harm Recall |
| --- | ---: | ---: | ---: | ---: |
| original proposal | 8 | 0.00318 | 0.02359 | 0.14746 |
| M1945 selector | 8 | **0.00068** | **0.00544** | 0.00000 |
| original proposal | 64 | 0.03001 | 0.15064 | 0.29333 |
| M1945 selector | 64 | **0.00114** | **0.00907** | 0.00000 |

The selector found 3 of 4,398 target assignments at top 8 and 5 at top 64. It
was worse than the already rejected fixed proposal.

## What Was Learned

Three statements can now be separated cleanly:

1. **Residual capacity exists.** Low-DF target atoms can distinguish missed
   positives from boundary competitors.
2. **Target versus explicit harm is observable.** The mean maximum
   target-minus-harm selector score is positive on every row, from `+0.882` to
   `+0.972`.
3. **Target versus the complete neutral field is not observable with this
   interface.** Thousands of out-of-sample neutral dimensions occupy the
   selected head, leaving both target and harm outside it.

The problem is not simply insufficient training depth. M1945 is a train-free
LODO audit and directly compares sampled separability with the true action
space. More optimizer steps cannot repair missing negative support.

## Route Consequence

The following are stopped:

- M1943-style fixed top-k proposal tuning;
- M1944 feature/classifier/threshold sweeps;
- neural training justified only by sampled target/harm AUC;
- native replay of the failed selector.

The following remain valid:

- M1934 `b1` as the fixed learned-sparse quality parent;
- M1934 `b1.125` as the stronger fixed-budget candidate;
- M1914/M1934 native one-index product shape;
- low-DF residual targets as a representation diagnostic.

Any future residual route needs a structurally different source construction
that has useful target coverage at its fixed deployable budget before learning.
It must not expose 7k-11k arbitrary dimensions and ask a shallow selector to
recover eight positives from sampled negatives. Until such a source exists,
the scientifically supported next work is broader generalization and ranking
improvement of the mature learned-sparse parent, not another residual gate.
