# M1510-M1518 Retrieval-Trained Sparse Route Final Report

Date: 2026-07-10

Final decision: close the tested external pooled SPLARE/SAE-latent source and
the frozen-BERT grouped SSR source. Do not launch M1514/M1515 scaling or native
index integration. Retain P1.3/M549U as the engineering baseline.

This is not evidence that all learned sparse retrieval is impossible. It is a
bounded negative result for the two representation sources tested under the
predeclared quality and corpus-cost gates.

## Stage disposition

| Stage | Evidence | Decision |
| --- | --- | --- |
| M1510 external SPLARE control | Real text-derived retrieval signal; full touch on NFCorpus, SciFact, and FiQA | semantic mechanism passes, deployment gate fails |
| M1510 public SSR control | No public checkpoint discoverable at cutoff | artifact unavailable, not a model failure |
| M1511 pooled failure localization | Removing LODO-universal atoms leaves touch at 98.36%-99.64% | failure is distributed through pooled vocabulary |
| M1512 pooled canary | M1510 already hits the `max_df near 1` stop | not launched |
| M1513 grouped SSR | Heldout pair/reconstruction improves; official quality far below P1 and exact retrieval touches full corpora | stopped after S2 |
| M1514/M1515 scaling/native | Required quality-cost canaries failed | not launched |
| M1516-M1516C post-hoc DF cap | Locked ArguAna row improves all quality metrics, but touch remains 63.64% | hard-cap branch stopped |
| M1518 primary DF-FLOPS | Better KL, pair accuracy, and head share; topDF/touch stay 1.0 | S1 fails |
| M1518B budget-matched DF-FLOPS | Exact prefix parity and stronger head reduction; topDF/touch stay 1.0 | source closed |

## What was established

### Retrieval supervision changes the learnability boundary

M1502 showed that qrels/action-derived free factors were not predictable from
frozen pooled PPLX roots. M1510 provides a different and positive result: when
retrieval supervision is placed inside representation construction, text can
produce sparse semantic features with real ranking utility.

The external reproduction beats dense and P1 on every NFCorpus quality metric
and beats P1 on every SciFact metric. Its FiQA result is weaker than dense and
P1, so it is not a uniform quality frontier, but it is sufficient to reject
the claim that useful sparse retrieval features are not text-learnable.

### Posting cost, not absence of semantics, is the primary pooled failure

The external sparse model touches every document on all three complete test
corpora. LODO removal of three to five corpus-invariant atoms barely changes
touch. M1516C then removes most postings with a locked global DF/TopK policy
and improves all ArguAna retrieval metrics, proving that the representation
contains both useful semantics and large amounts of unnecessary support.

The remaining 63.64% ArguAna touch comes from the union of many medium-DF
atoms. It is not repairable by deleting one universal stop feature or by an
inference-only threshold.

### Grouping alone does not repair the frontier

M1513 preserves token groups and trains reconstruction, AuxK, sparse
contrastive, and supervised MaxSim losses. Pair accuracy rises from 0.3576 to
0.5114 and reconstruction cosine from 0.5428 to 0.8343. Nevertheless, trained
NFCorpus NDCG@10 is only 0.0764 and SciFact NDCG@10 only 0.0602. Exact K=32
retrieval touches both full corpora; K=4 coarse support touches 85.38% and
99.90%, respectively.

Training is real, but the frozen generic-BERT representation is neither
retrieval-pretrained enough nor corpus-selective enough. More steps on the
same source are not justified.

### DF-aware training changes mass but not the deployment boundary

M1518 verifies the DF-FLOPS implementation against stored full-corpus
postings, then uses a causally paired continuation from the M1510 adapter.
The primary run improves final KL by 0.01434, pair accuracy by 1.95 points,
and head share by 0.01131 relative to FLOPS. TopDF and mean touch remain 1.0.

The paper-supported M1518B correction scales DF-FLOPS only after its first DF
refresh. Steps 0 and 100 remain exact, and the first active total loss exactly
matches the ordinary-FLOPS budget. It reduces final head share further, from
0.16258 to 0.13316, but leaves one latent at DF=100%, another at 99.83%, and
mean touch at 100%. Final KL is also 0.01461 worse than FLOPS.

The last universal atom already receives the maximum DF weight. Lowering the
activation cutoff cannot increase that gradient, so an alpha correction is
not authorized. Longer training or another threshold grid would violate the
stop contract without introducing a new mechanism.

## Truth boundaries

- The SPLARE result is an independent reproduction, not an official public
  checkpoint. Its model card does not prove clean exclusion of all BEIR rows,
  so the rows are OOD controls rather than formal clean-heldout claims.
- SSR released code but no checkpoint at the evaluation cutoff. M1513 is a
  local MS MARCO training result and does not claim to reproduce the paper's
  unavailable model.
- BEIR qrels are used only for final quality measurement. No M1516 threshold,
  M1518 loss scale, checkpoint, or gate is selected from BEIR qrels.
- BM25 supplies training negatives only. It never enters M1518 inference
  scores.

## Final scientific conclusion

The route answered the original question:

1. Prior failures were partly training-method failures. Retrieval-trained
   sparse representations can learn useful text-derived retrieval semantics.
2. Unified sparse representation is not disproven in general.
3. The tested SAE-latent sources are unsuitable as bounded-cost unified
   postings. Pooled and grouped variants both retain corpus-universal
   structural support.
4. Ordinary sparsity, post-hoc DF pruning, paper-control DF-FLOPS, and a
   causally budget-matched correction all fail the same native cost boundary.

A future route must change the representation source before scaling. It needs
retrieval semantics and corpus-background separation to be structural in the
output vocabulary, rather than asking a pooled SAE latent field to suppress
universal components after the fact. That is a new research hypothesis, not
M1518C or continuation of this checkpoint.

## Verification and artifacts

- `docs/research-sae/reports/m1500-m1599/ii42-m1510-m1511-external-sparse-control-report.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1513-ssr-grouped-canary-report.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1516-m1516c-splare-df-budget-final-report.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1518-df-flops-paired-smoke-report.md`
- `runs/m1518_df_flops_paired_smoke_v1/m1518_s1_paired_gate.json`
- `runs/m1518b_df_flops_budget_match_v1/m1518_s1_paired_gate.json`
- 26 focused M1516/M1518 tests pass.
- Relevant Python files pass `py_compile` and Ruff.
- Relevant shell runners pass `bash -n`.
- `git diff --check` passes.
- No M1518 tmux, Docker, or Python process remains on spark-1.
