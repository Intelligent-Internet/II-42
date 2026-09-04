# M1545 Additive Semantic/Lexical Rescue

## Decision

**Close semantic-route rescue and do not train a scorer.**

Adding up to 5% unseen BM25 candidates to the frozen 15% dual semantic route
does not exceed equal-20% dense candidate capacity on any canary. The
predeclared capacity gate is `0/3`; therefore the listwise-training branch is
not authorized.

Commit under test: `98b7fea0`.

ClearML task: `37684687b332406a82382817372d1269`.

## Equal-Budget Capacity

| Dataset | Dense top20% CUB | BM25 top20% CUB | Additive union CUB | Delta vs best |
| --- | ---: | ---: | ---: | ---: |
| nfcorpus | 0.567643 | 0.350565 | 0.544651 | -0.022992 |
| scifact | 0.990000 | 0.980000 | 0.990000 | 0.000000 |
| fiqa | 0.980556 | 0.869187 | 0.976667 | -0.003889 |
| macro | 0.846066 | 0.733250 | 0.837106 | -0.008960 |

The union is bounded (`0.1926` macro unique-candidate ratio), but BM25 does not
add enough unseen positives to compensate for route misses. An oracle or
learned scorer cannot rank positives that are absent from this union.

## Retrieval Quality

| Macro source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB | Union |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense top20% | 0.667473 | 0.562316 | 0.767131 | 0.757520 | 0.846066 | 0.2001 |
| BM25 top20% | 0.549246 | 0.454358 | 0.652973 | 0.643514 | 0.733250 | 0.1650 |
| route15 tail256 | 0.648264 | 0.544473 | 0.721202 | 0.730274 | 0.783262 | 0.1501 |
| additive tail256 | 0.669063 | 0.562879 | 0.759776 | 0.758627 | 0.837106 | 0.1926 |
| additive P1 RRF a0125 | 0.665760 | 0.559283 | 0.761434 | 0.756838 | 0.837106 | 0.1926 |
| additive exact-dense upper | 0.669331 | 0.562849 | 0.760748 | 0.759066 | 0.837106 | 0.1926 |

The tail-only and exact-dense-upper rows are again almost identical. Ranking
is not the limiting component. The fixed P1 fusion moves Recall slightly but
does not beat dense, and MAP remains lower.

## Final Route Conclusion

M1545 completes the bounded M1540-M1545 investigation:

- M1540: paper-shaped Latent Terms has real semantic residual signal, but an
  exact query touches 87–96% of documents;
- M1541: signed coordinates + tail256 reproduce dense quality only after a
  full-union scan;
- M1542: document route atoms are the strongest honest access source, reaching
  94.3% dense Recall at 15% union, but they are not dense-equivalent;
- M1543: lexical substitution inside a fixed 15% budget is not complementary;
- M1544: coordinate block pruning is worse and costs about 54–58 KB of block
  summaries per document;
- M1545: additive lexical rescue still does not exceed equal-budget dense
  candidate capacity.

There is no justified training branch left in this family. More epochs, loss
weights, alphas, route counts, selectors, gates, or rerankers would optimize
inside a candidate set already proven insufficient.

## Product Recommendation

For final recall, keep the semantic access structure ANN-shaped:

1. use the frozen dense encoder with VectorChord/another proper ANN index;
2. keep BM25 as the lexical inverted source;
3. perform one auditable global fusion/ranking layer over their candidate
   union;
4. retain M549U/P1 atoms where they add interpretability or native lexical
   evidence, but do not claim they replace dense access;
5. evaluate the product through the native DB path on held-out official rows.

If a single physical index remains a hard product constraint, it should be a
hybrid index with separate lexical and vector access paths under one lifecycle,
not one forced posting algebra pretending to preserve continuous dense
geometry.

## Artifacts

- `runs/m1545a_additive_rescue_v1/summary.json`
- `runs/m1545a_additive_rescue_v1/summary.md`

