# M1553 P1 Single-Index Protected Tail Report

## Result

Decision: **retain the single-index architecture, but do not promote the
current unrestricted tail policy**.

M1553 is the first evaluation in this line where semantic and lexical
candidates come from the same model-backed P1 index regclass.  The index uses
M549U signed semantic postings and its existing lexical posting surface.  The
runtime preserves P1 semantic ranks 1-99 and uses lexical rank to select the
rank-100 tail document.

## Validation ladder

### Shared15

The 15-row, mostly 2K-document shared15 surface passed:

- Recall@100: `+0.005272`;
- CUB: `+0.010212`;
- MAP@100: `+0.000408`;
- NDCG@10 / MRR@20: unchanged;
- row harms: `0/15`.

### Official full-corpus canary3

Full nfcorpus/scifact/fiqa also passed:

- Recall@100: `+0.012847`;
- CUB: `+0.034604`;
- MAP@100: `+0.000217`;
- NDCG@10 / MRR@20: unchanged;
- row harms: `0/3`.

### Official full-corpus 9/15

The 26,916-query native run produced a strong macro improvement:

- Recall@100: `+0.067607`;
- CUB: `+0.137846`;
- MAP@100: `+0.000898`;
- NDCG@10 / MRR@20: unchanged.

The gains were especially large on cqadupstack (`+0.145719` Recall), quora
(`+0.389845`), and webis-touche2020 (`+0.028501`).  However, arguana regressed
by `-0.000714` Recall and `-0.000007` MAP.  The hard row-safety gate therefore
correctly rejected promotion.

## Failure mechanism

The current policy sorts the entire semantic-plus-lexical tail by BM25 rank.
It may therefore promote a document that was already in the semantic tail,
rather than a document newly admitted by the lexical residual namespace.  On
arguana, where candidate CUB gain is exactly zero, that movement can displace a
relevant semantic rank-100 document without adding new relevant capacity.

This is an implementation/design mismatch, not evidence against the unified
index:

- the intended residual head contributes BM25-unique documents;
- semantic tail documents should retain semantic order;
- if no unique lexical document is available, no boundary swap is needed.

## Next gate

M1555 will admit only lexical-unique documents at rank100.  It is authorized
because the change follows directly from the source decomposition and the
observed arguana failure, not from qrels threshold tuning.

Promotion still requires positive macro CUB/Recall/MAP, exact NDCG/MRR
preservation, and no dataset-row harm on the same official9 surface.
