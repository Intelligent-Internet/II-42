# M1722-M1728 Residual And Additive Final Report

## Verdict

This sequence produces one real advance and one hard stop:

- **advance:** dense-minus-BM25 residual capacity can be exposed by adding
  frozen query-logit keys without changing the base source;
- **stop:** the current one-hop additive posting score cannot rank that
  capacity, and deeper training has already been falsified across matched
  controls and full-scale data.

No checkpoint or native product is promoted.

## Evidence Chain

| Stage | Result | Meaning |
| --- | --- | --- |
| M1722 | stronger residual oracle, worse frozen observability | teacher change alone is insufficient |
| M1723 | joint residual source gains on 4k, quality optimum over cost | source movement has signal and a frontier |
| M1724 | minibatch read proxy underestimates exact cost | cost must use corpus DF |
| M1725 | exact train DF works, validation load shifts | canary data is too small for load generalization |
| M1726 | full scale closes load gap, every trained checkpoint loses to initialization | not undertraining; joint movement is destructive |
| M1727 | frozen expansion gains `+0.02369/+0.03980` O@100/O@256 at `0.179909x` | base-safe residual admission is real |
| M1728 | native logit score keeps only `15.0%/23.9%` of those gains | score resolution is the remaining structural blocker |

## Answer To The Training-Depth Hypothesis

The project did pay the larger training cost needed to test this concern:

- full 10k/1k query pools;
- 88,992/8,988 disjoint documents;
- exact BM25 on both corpora;
- complete train-corpus DF refresh;
- dense control and residual arms through 1,600 updates;
- exact train and validation read accounting at every checkpoint.

Scale reduced the train-validation read gap by about 64%, proving that more
data solved a genuine estimation issue. It did not create a delayed quality
pass. The best full residual O@100 occurs at step 400 and remains below epoch
zero; all later checkpoints also remain below. The route is not being stopped
because training was cheap or short.

## Correct Product Interpretation

The valid shape discovered here is:

```text
frozen lexical postings
  + frozen semantic base postings
  + budgeted frozen-logit expansion postings
  -> strong candidate surface
```

What is missing is a posting-local score with enough within-list resolution.
M1727's exact dense rerank supplies that resolution externally; M1728 removes
it and the score collapses from candidate-upper O@100 `0.889080` to native
O@100 `0.447890`.

Therefore the following would be scientifically invalid next steps:

- another residual-router depth or seed;
- alpha, normalization, threshold, probe, or loss sweeps;
- calling exact dense reranking part of the unified posting encoder;
- evaluating qrels before score representability passes.

## M1730 Follow-Up Closure

M1730 executed the bounded second-stage option with a fixed 96-byte residual
payload and compact safe geometric blocks. It failed both independent gates:

- compact all-candidate O@100 retained only `79.17%` of exact;
- safe summaries opened every selected block and decoded every candidate.

The full result is recorded in
`docs/research-sae/reports/m1700-m1799/ii42-m1730a-bounded-residual-block-report.md`. This closes the proposed
single-vector bounded residual repair without changing the M1727 candidate
result.

## Architectural Choice

There are only two evidence-backed continuations:

1. **Adopt a genuinely multi-vector late-interaction representation.** This is
   a new model/index contract rather than a compressed single-vector repair.
2. **Keep the current product contract.** Stop the dense-replacement objective
   and retain the learned-sparse baseline; M1728 and M1730 reject both one-hop
   and bounded residual scoring for the tested dense-root representation.

The M1727 frozen expansion may be retained as a candidate-source component
for option 1. It must not be presented as an independent retrieval model.

## Final Status

M1722-M1730 is complete. The residual teacher, exact cost tooling, and frozen
expansion audit are retained. Joint-source, one-hop additive scoring, and the
fixed bounded residual-block repair are closed. No further training is
authorized on this representation.
