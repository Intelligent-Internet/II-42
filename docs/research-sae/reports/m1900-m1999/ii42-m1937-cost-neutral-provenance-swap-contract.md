# II-42 M1937 Cost-Neutral Provenance Swap Contract

## Question

M1936 showed that learned-expansion postings carry most of the useful
`b1->b1.125` tail but are too numerous to add directly. M1937 asks whether
exact lexical coverage can fund that expansion at the existing `b1` index
size.

## Fixed Transformation

For each document independently:

1. identify `b1` semantic postings whose dimension occurs in the document's
   Granite tokenizer input;
2. identify learned-expansion postings in the canonical `b1->b1.125` tail;
3. add the highest-impact learned expansions;
4. remove the same number of lowest-impact input-aligned `b1` postings.

The number swapped is the smaller of the two available sets in that document.
There is no swap ratio, threshold, dataset-specific parameter, qrel, or
retrieval-time feature. Every document and the complete index preserve their
exact `b1` semantic posting count.

## Frozen Variables

M1914 impacts and source support, M1931 query calibration, exact lexical
postings, tokenizer/truncation, candidate depth, and one-index additive
scoring remain unchanged. The full nested `b1.125` route is evaluation-only
and provides the available-gain reference.

## Progressive Gate

Run SciDocs first. Authorize unchanged execution on Quora and TREC-COVID only
when the fixed swap:

- preserves the exact `b1` semantic posting count;
- keeps macro NDCG@10, MAP@100, and MRR@20 at or above 99.5% of `b1`;
- does not reduce macro Recall@100 or CUB@1000;
- retains at least 25% of the complete nested gain in Recall or CUB;
- keeps mean posting touches at or below 1.05 times `b1`;
- passes every existing row-level Recall/head safety floor.

A three-row pass authorizes seven-row LODO and exact native replay, not model
training. Failure closes input-token provenance as a standalone support
allocation teacher. Do not run a smaller swap ratio or train a selector to
imitate a failed swap.
