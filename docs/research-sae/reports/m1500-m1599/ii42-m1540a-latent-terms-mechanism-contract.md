# M1540A Latent Terms Mechanism Contract

## Decision Being Tested

M1540A tests whether the M1520C failure came from the representation source or
from four concrete departures from the Latent Terms mechanism:

1. hierarchical clustering instead of a reconstruction-trained Top-K SAE;
2. max aggregation instead of token-level sum aggregation;
3. sparse dot-product scoring instead of BM25 over latent terms;
4. explicit removal of common features despite evidence that the heavy
   Zipf-like head remains useful to latent BM25.

This is a bounded causal mechanism test, not a claim to reproduce the paper's
30B-token training run.

## Locked Surface

- Frozen backbone: `BAAI/bge-base-en-v1.5` from M1520C.
- Qrels-free train corpus: the same shared MS MARCO document file.
- Training budget: 262,144 token states selected from at most 4,096 documents.
- Model: 32,768 latent Top-K SAE, K=16, tied decoder/encoder initialization.
- Pooling and transform: token sum followed by square root.
- Scoring: latent BM25 with global untuned `k1=8`, `b=0.7`.
- Canary rows: NFCorpus, SciFact, and FiQA on the exact M1520C corpora.
- Bounded surface: document K=96, query K=24, candidate K=1000.
- Qrels are read only after the SAE, postings, index, and rankings are fixed.

No cross-encoder, qrels, BM25 labels, dataset ID, per-row parameter, routing,
or retrieval loss enters training.

## Gates

The first run is allowed to scale only if all conditions hold:

1. Training is healthy: reconstruction cosine at least 0.75 and at least 25%
   of latent features activate.
2. On at least 2/3 rows, full latent BM25 retains at least 80% of exact BGE
   candidate upper bound and bounded residual recovery improves over M1520C by
   at least 0.05 absolute.
3. On at least 2/3 rows, bounded latent BM25 reaches at least 50% recovery of
   dense-recoverable BM25 misses at no more than 30% mean exact touch.

The 50% recovery threshold is only a scale gate. The final URSI promotion
threshold remains 90% and cannot be weakened by this experiment.

## Stop Rules

- Do not tune BM25 parameters, K, latent size, or loss weights after the run.
- If training is healthy but source quality fails, stop this BGE source.
- If source quality passes but exact cost fails, record a source-only signal;
  do not call it an engine breakthrough or silently replace the cost gate with
  an approximate-index claim.
- Only a conjunctive pass authorizes a larger qrels-free token corpus.

## Literature Boundary

The design follows the mechanism in *Latent Terms: Dense Retrievers Contain
Trivially Extractable BM25-ready Zipfian Vocabularies* (arXiv:2605.29384):
Top-K SAE reconstruction on frozen retriever token states, sum pooling,
sublinear activation transform, and BM25 scoring. The paper trains on 30B
tokens; M1540A deliberately does not claim equivalent scale or quality.

