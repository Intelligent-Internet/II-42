# M1552 Frozen Lexical Residual Report

## Result

Decision: **stop the pooled-dense residual head; do not run sparse-head or
LoRA variants on this teacher**.

M1552 trained a rank-32 dual-bilinear capacity ceiling over frozen official
1024-dimensional dense-root embeddings.  The model saw no qrels.  For each
query it learned, with listwise candidate competition, to select the M1549
rank-100 document from dense tail and lexical hard negatives.  Checkpoint
selection used a qrels-free validation split; qrels were loaded only for final
protected-tail replay.

The full LODO run used 1,271 queries, was tracked in ClearML task
`cf0aaaf3eef844bbbc369208c10c9da6`, and ran on spark-1 without touching
spark-2.

## Evidence

| Holdout | Target@100 | Recall gain kept | Result |
| --- | ---: | ---: | --- |
| nfcorpus | 0.0062 | 0.0386 | fail |
| scifact | 0.0100 | 0.0000 | fail |
| fiqa | 0.0062 | -0.7407 | fail |
| Macro | 0.0075 | 0.0033 | fail |

Random target selection among 173 tail candidates is about `0.0058`.  The
held-out model is therefore only marginally above random.  It retained `0.33%`
of the M1549 macro Recall gain.

This is not a shallow-training failure:

- training loss fell close to zero;
- qrels-free validation target accuracy reached `4.5%`, `7.2%`, and `12.5%`
  in the three folds;
- held-out datasets collapsed back to random;
- the unconstrained dual query/document projection is a capacity ceiling for
  a smaller shared sparse head.

The failure is cross-corpus generalization: the pooled dense embedding does not
retain enough stable lexical identity to infer which BM25-only document should
occupy the protected boundary.

## Consequence

Do not spend more compute on:

- a shared sparse projection over the pooled 1024-vector;
- deeper residual MLPs over the same input;
- LoRA or backbone thawing for the M1549 lexical target;
- more epochs on the same three-corpus teacher.

The next justified architecture is token-aware:

1. keep M549U/P1 semantic signed postings from the dense root;
2. publish exact lexical term postings directly from the tokenizer/text path;
3. combine both namespaces in one II-42 model/index;
4. preserve the semantic top99 and use lexical evidence only for tail
   admission.

This is still one encoder checkpoint and one unified posting index.  It avoids
the scientifically unsupported step of asking a pooled semantic vector to
reconstruct rare lexical identity that is already available exactly in the
input text.
