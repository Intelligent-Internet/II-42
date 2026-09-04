# II-42 M1936 Semantic-Tail Provenance Contract

## Question

M1935 showed that document frequency cannot isolate a cheap, high-utility
subset of the semantic postings added between `b1` and `b1.125`. M1936 tests a
different structural hypothesis from the M1930 residual design:

> semantic dimensions generated from tokens absent from the source document
> may carry less lexically redundant expansion evidence than dimensions
> already present in the model input.

This is an observability audit, not model training. Input provenance is known
at encoding time and requires no qrels, retrieval results, or corpus-specific
threshold.

## Frozen Variables

- IBM Granite 30M sparse revision
  `ad82b1fd09541c998c8d45045d601c51fdb8a9b7`;
- M1914 impact transform and top-192/top-50 source support;
- canonical nested `b1` to `b1.125` document expansion;
- M1931 `rms_m4` query calibration;
- exact II42 lexical postings and one additive sparse dot product;
- candidate depth 1,000;
- the model tokenizer, truncation at 512 tokens, and special-token exclusion.

Only incremental-tail document postings may be partitioned. Every `b1`
posting remains active.

## Routes

Evaluate exactly four cumulative surfaces:

1. `b1`;
2. `input_aligned_tail`: add only tail dimensions found in that document's
   tokenized input;
3. `learned_expansion_tail`: add only tail dimensions absent from that
   document's tokenized input;
4. `nested_b1.125`: add the complete tail.

Input alignment is a SPLADE-style source-provenance proxy. It is not claimed
to be identical to exact BM25 redundancy because Granite dimensions are
subword vocabulary items while II42 lexical terms use the native tokenizer.

## Progressive Gate

Run SciDocs first. A partial route authorizes unchanged execution on Quora and
TREC-COVID only when it:

- retains at least 70% of the full NDCG@10, MAP@100, and MRR@20 gains;
- does not reduce macro Recall@100 below `b1`;
- retains at least 50% of the full CUB@1000 gain;
- uses at most 75% of the full added postings;
- uses at most 75% of the full added posting touches;
- keeps each row above the M1935 Recall and head-metric safety floors.

If both provenance routes pass, select the one with lower added-touch
fraction. The route and all gates then remain frozen for the three-row run.

## Stop Rules

Stop this family without a finer provenance rule when neither route passes
SciDocs or when the selected route fails any three-row gate. Do not train a
classifier to imitate a failed partition.

A three-row pass authorizes only a seven-row LODO and exact native replay. It
does not yet authorize residual-head training. Training requires the same
source to survive those broader gates and to expose a target not already
implemented exactly by the deterministic publisher.
