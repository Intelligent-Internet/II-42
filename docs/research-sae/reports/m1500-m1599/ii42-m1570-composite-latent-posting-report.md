# M1570 Composite Latent Posting Report

## Decision

**Stop product/composite latent posting keys.**

The source achieves the intended low-DF inverted-index shape, and its
diagnostic cell oracle contains almost all dense top256 documents. The
deployable product-centroid ordering is nevertheless far below both the
primary gate and the restricted training bridge. Do not train a query head,
learned codebook, OPQ variant, or another cell-count/split configuration.

## Surface And Integrity

- Official FiQA: 57,638 documents and 648 queries.
- One 65,536-key composite posting namespace.
- Exactly one posting per document.
- 19,484 non-empty keys; mean non-empty load `2.96`, p95 `9`, maximum `80`.
- Maximum DF ratio: `0.001388`.
- Normalized cell-load entropy: `0.960802`.
- Orthogonal dense-dot parity maximum error: `1.76e-6`.
- Product reconstruction cosine: `0.826177`.
- Qrels-free basis SHA-256:
  `6e8e8381ca3497149705a9d8e756e31d1d624e8f1d87ba7290839466730e2ca3`.
- Qrels-free candidate-surface SHA-256:
  `618f3e3b8f14aa7da08748fbb047dbe036d5dfcb8664ee72423a3f641223796f`.
- ClearML task: `6d4233e8fdc24b53b19d3e3e72c5e682`.
- Runtime: 47.5 seconds.

## Frontier

| Budget | Policy | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | MAP@100 | MRR@20 | CUB | Reads | Cell probes |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1,000 | deterministic | 0.635031 | 0.579645 | 0.516457 | 0.489570 | 0.285633 | 0.231743 | 0.372124 | 0.567367 | 0.017350x | 851.2 |
| 1,000 | cell oracle | 1.000000 | 1.000000 | 0.897021 | 0.733984 | 0.400339 | 0.337638 | 0.485244 | 0.848188 | 0.017350x | 185.0 |
| 1,256 | deterministic | 0.683488 | 0.629198 | 0.568269 | 0.517030 | 0.296326 | 0.242179 | 0.386727 | 0.601844 | 0.021791x | 1,063.7 |
| 1,256 | cell oracle | 1.000000 | 1.000000 | 0.967846 | 0.733984 | 0.400339 | 0.337638 | 0.485244 | 0.859857 | 0.021791x | 232.5 |
| 2,048 | deterministic | 0.781790 | 0.733441 | 0.680381 | 0.582706 | 0.326607 | 0.267758 | 0.416000 | 0.695343 | 0.035532x | 1,699.1 |
| 2,048 | cell oracle | 1.000000 | 1.000000 | 0.999994 | 0.733984 | 0.400339 | 0.337638 | 0.485244 | 0.886358 | 0.035532x | 383.1 |

At budget 1,000, the deterministic composite source is worse than the frozen
M1565 route source by `-0.228858` O@100 and `-0.185854` O@256. The restricted
training bridge requires `0.90/0.82` at budget 1,256; the observed values are
only `0.629198/0.568269`.

## Interpretation

M1570 separates key granularity from query observability. Product cells are
small, balanced, cheap, and capable of containing the correct neighbors. But
the sum of two centroid dot products does not identify those cells for BGE
text embeddings. This agrees with the known weakness of inverted
multi-indices on highly entangled deep descriptors.

The oracle result is not permission to train. Its cell ordering directly sees
the evaluation query's exact dense ranks, while the deployable score is more
than 0.32 O@100 below the bridge. M1561/M1562 already showed that query-derived
route labels can memorize a strong head without transferring. Training here
would repeat that failure on a finer vocabulary.

M1565 and M1570 now close the two mutually exclusive quantization shapes:
coarse global cells preserve more query observability but insufficient dense
coverage, while fine product cells preserve oracle capacity but destroy
deployable cell ordering.

## Next Structural Gate

One mathematically different posting source remains justified:
overlapping random-hyperplane LSH tables. Unlike centroid or product
quantization, random-hyperplane collision probability is monotonic in cosine
similarity. Documents publish several low-DF bucket keys; queries probe exact
and low-margin neighboring buckets. This retains one inverted namespace and
requires no external ANN path.

The next experiment must use one analytically selected table/bit shape, count
duplicate posting reads, and stop if the fixed source does not beat M1565 at
the same candidate budget. It must not become a hash-count or Hamming-radius
grid. If this overlapping source also fails, the single-hop semantic posting
candidate branch has no remaining evidence-backed construction.
