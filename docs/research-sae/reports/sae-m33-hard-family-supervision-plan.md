# SAE M33 Hard-Family Supervision Plan

Status: active after M32 failed the robustness gate.

## Summary

M32 proved that larger clean BEIR train/dev data helps aggregate quality, but
does not solve hard-family collapse. The largest remaining blocker is
`trec-covid`, with secondary failures on `msmarco` and `dbpedia-entity`.

M33 therefore stops global weight/loss sweeps and adds targeted supervision for
the missing query family. The goal is not to use `trec-covid` test qrels for
training. The goal is to expose the query encoder to biomedical/claim-heavy
language while preserving the M31/M32 final-ranking objective:

```text
query text -> SAE atoms
BM25 token atoms + SAE latent atoms -> single sparse evidence engine
```

## Hypothesis

The current model can learn the final ranking surface when the query family is
represented in training. It fails when a held-out family has semantic behavior
that the text-to-atoms encoder never sees. `trec-covid` has no official
train/dev split in the local BEIR cache, so M32 cannot teach that family even
though the teacher can solve it.

## First M33 Arm

Build a pseudo-query training artifact from `trec-covid` corpus text:

- Use document titles or first useful sentences as pseudo queries.
- Use only self-document pseudo qrels.
- Do not use official `trec-covid` test query text or qrels as labels.
- Mark the artifact as `quality_claim_allowed=false`.
- Merge it with the M32 general train root and rerun the teacher-anchor
  final-ranking trainer.

This arm is intentionally conservative. It tests whether biomedical vocabulary
and claim-style phrasing exposure helps the query encoder without contaminating
the held-out `trec-covid` evaluation qrels.

## Acceptance Gate

M33 can only continue toward doc-side or SQL work if it materially improves the
hard-family collapses:

| Gate | Requirement |
| --- | --- |
| `trec-covid` | NDCG@10 and MAP@100 move materially toward teacher, not merely aggregate mean improvement |
| `msmarco` / `dbpedia-entity` | no new or worse collapse beyond M32 teacher-anchor |
| Aggregate | full15 mean does not regress below M32 teacher-anchor |
| Cost | SAE postings and candidate docs remain within the M32 teacher-anchor profile unless quality gain is decisive |

If the pseudo-query arm does not help, the next valid move is not another
weight sweep. It is either a stronger query encoder trained with the same
final-ranking objective, or a teacher-neighborhood distillation dataset built
from biomedical/claim-heavy corpora without test-qrel leakage.

## Test Plan

1. Build a smoke pseudo artifact.
2. Build the full `trec-covid-pseudo` artifact.
3. Materialize Snowflake embeddings and `shared_sae_8192_64` teacher latents.
4. Symlink M32 general train datasets plus the M33 pseudo dataset into one
   merged train root.
5. Run the M32 teacher-anchor training configuration with the extra pseudo
   dataset.
6. Compare against M32 teacher-anchor on full15 aggregate, hard collapse table,
   and physical cost.

## Decision Rule

If `trec-covid` remains close to BM25 and far from teacher, M33 should record a
negative result and switch from pseudo self-qrels to either:

- biomedical teacher-neighborhood distillation; or
- a stronger query encoder with the same final-ranking and no-collapse gates.
