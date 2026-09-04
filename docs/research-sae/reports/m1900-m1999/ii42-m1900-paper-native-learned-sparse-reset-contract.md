# M1900 Paper-Native Learned-Sparse Reset Contract

Date: 2026-07-12

Status: **complete through M1913; see
`docs/research-sae/reports/m1900-m1999/ii42-m1900-m1913-learned-sparse-reset-consolidated-report.md`**

## Objective

Restart the learned-sparse route from published, retrieval-native systems rather
than another project-specific posting loss. The first new question is narrow:

> Under the same DistilBERT initialization, training rows, teacher scores,
> optimization budget, and heldout evaluator, does a TopK-SAE latent vocabulary
> produce a better quality/cost frontier than a standard SPLADE MLM vocabulary?

This is a mechanism comparison. It is not yet a paper-result reproduction or a
product promotion.

## Prior Evidence That Remains Binding

- M1640 already reproduced a strong public learned-sparse control in an exact
  inverted-index evaluator. The OpenSearch sparse checkpoint is the current
  product-eligible control; it must not be replaced by a weaker toy baseline.
- M1700 showed that a short retrieval-native curriculum can improve quality,
  while longer training opens high-DF terms and increases decoded postings.
  Training loss alone cannot select a checkpoint.
- M1549 remains the closest final-quality teacher, but it is not a one-index
  product. It is a quality reference, not the architecture used in this test.
- M1720-M1820 showed that score-preserving latent postings and token routing do
  not automatically provide candidate admission. A new representation must
  earn retrieval quality before engine optimization.

## Published Starting Points

| Route | Public artifact | Role in M1900 |
| --- | --- | --- |
| SPLADE-v3 | Paper, code, public checkpoint | Mature vocabulary-sparse quality control |
| OpenSearch sparse v2 | Apache-2.0 checkpoint | Existing product-eligible frozen control |
| SAE-SPLADE | Paper and training code; no public checkpoint or explicit repository license found | New paired mechanism candidate |
| LACONIC | Paper, code, public 1B checkpoint; no explicit repository license found | Scaling reference, not the first reproduction target |
| SPLARE | Paper; no public checkpoint found | Architecture evidence only |
| DF-FLOPS | Paper/code lineage | Cost diagnosis if high document frequency recurs |

The repository audit pins SAE-SPLADE commit
`2056602254ef8ab2162e1f7424a570596b448f71`. The pinned `pyproject.toml` and
`uv.lock` hashes are recorded by the audit output. Missing licenses are a hard
product boundary, not a reason to suppress a research reproduction.

## M1900 Artifact Gate

Before model training:

1. Verify the pinned upstream commit and dependency-file hashes.
2. Record whether code, configuration, checkpoint, data recipe, and license are
   independently available.
3. Run the official experiment graph in an isolated environment. A dry-run
   that starts downloads is recorded as side-effecting and is not repeated
   against an ephemeral cache.
4. Keep all upstream downloads under a persistent run directory.

M1900 passes as a *research mechanism source* when code, configuration, and the
pinned revision verify. It cannot pass a product-adoption gate without an
explicit compatible license and a reproducible checkpoint or training run.

## M1901 Paired Canary

### Shared Inputs

- Backbone: `distilbert/distilbert-base-uncased` masked-language model.
- Backbone revision: `12040accade4e8a0f71eabdb258fecc2e7e948be`.
- Training/validation: fixed M1518 MS MARCO teacher rows.
- Each row: one query, one positive, eight negatives, and fixed teacher scores.
- Identical row order, negative order, optimizer budget, IR loss, FLOPS ramp,
  token lengths, precision, and evaluation checkpoints.
- BEIR qrels are not used for training or checkpoint selection.

### Branch A: Standard SPLADE

Use non-negative MLM logits, token max pooling, and `log1p`, followed by the
same distillation and FLOPS losses as Branch B.

### Branch B: SAE-SPLADE

1. Freeze DistilBERT and train a TopK SAE reconstruction head on the fixed text
   pool. Use width 65,536, token TopK 8, tied encoder/decoder initialization,
   decoder-row normalization, and the published auxiliary reconstruction term.
2. Fine-tune the retrieval encoder with token TopK 8, token max pooling, and
   `log1p`.
3. Use the paper-native IR objective: candidate-set KL, positive-margin MSE
   weight 0.05, query/document FLOPS weights `0.06/0.04`, and the absolute
   6,000-step quadratic FLOPS ramp. The short canary does not compress this
   regularization ramp into its 100-step budget.

The canary may use fewer steps and shorter documents than the paper. Any such
run is labelled `mechanism_canary`; it cannot be called a full SAE-SPLADE
reproduction.

The first `M1901a` environment closure compressed the FLOPS ramp to 20 steps
and reversed the formal query/document coefficients by following the debug
configuration. Standard SPLADE collapsed to about one document nonzero, so
that comparison is retained only as a schedule-failure diagnostic. `M1901b`
uses the formal `normal_base` and `baseline_splade/normal` values above.

## Canary Metrics

Heldout metrics are computed on all nine candidates per row:

- teacher-distribution KL;
- positive-margin MSE;
- teacher top-1 agreement;
- positive top-1 and pairwise accuracy;
- row-wise Spearman correlation;
- query/document mean nonzeros and FLOPS;
- sampled maximum document frequency;
- finite-gradient and reconstruction diagnostics.

## Decision Gate

Authorize a native FiQA retrieval replay only if a trained SAE checkpoint:

1. improves its own initialization on KL or margin MSE;
2. does not reduce positive top-1 or pairwise accuracy by more than 0.5 points;
3. matches or beats the trained standard SPLADE branch on at least one teacher
   fit metric without losing more than 0.5 points on positive ranking;
4. keeps document mean nonzeros, FLOPS, and sampled max DF at or below `1.15x`
   the standard branch;
5. is selected by the locked heldout gate, never by training loss.

Passing M1901 authorizes one exact native FiQA replay. It does not authorize a
full paper-scale run.

## Expansion And Stop Conditions

- If M1901 fails teacher fit and ranking, stop SAE-SPLADE on this root. Do not
  search widths, TopK values, or new losses.
- If quality passes but cost fails through max DF, run one DF-FLOPS diagnosis;
  do not use post-hoc pruning as the primary result.
- If paired heldout passes, run exact FiQA against standard SPLADE and the
  frozen OpenSearch control. Expand to shared3 only when FiQA quality and
  native cost both pass.
- A paper-scale reproduction is justified only after the mechanism and native
  gates pass. It must use persistent data, ClearML tracking, and an explicitly
  budgeted full training recipe.

## Required Artifacts

- `m1900-artifact-audit.json` and `.md`
- pinned environment freeze, SHA, and model revision
- M1901 `summary.json`, `summary.md`, checkpoints, and ClearML task IDs
- native replay JSON/Markdown only when the paired gate authorizes it
- a conclusion-bearing report that distinguishes mechanism, paper
  reproduction, and product eligibility
