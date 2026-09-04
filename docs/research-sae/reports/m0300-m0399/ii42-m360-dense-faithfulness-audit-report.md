# ii42 M360 Dense Faithfulness Evidence Audit

## Scope

This is a CPU-only audit over existing local JSON artifacts. It does not start new training, does not use GPU, and does not touch `spark-1` or `spark-2` workloads.

The question is whether existing evidence already answers: can the current SAE/atom representation preserve dense retrieval behavior without relying on BM25 or downstream scorer rescue?

## Dense References

| Surface | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 | NDCG/dense | MAP/dense |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| small3 | exact_dense | 0.743283 | 0.615236 | 0.532963 | 0.446425 | 1.000 | 1.000 |
| expansion5 | exact_dense | 0.773886 | 0.428000 | 0.400051 | 0.303155 | 1.000 | 1.000 |
| expansion5 | product_vectorchord_dense | 0.693498 | 0.486705 | 0.421108 | 0.328919 | 1.053 | 1.085 |

## Atom-Only Rows

| Surface | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 | NDCG/dense | MAP/dense |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| small3 df_le_0p25 | atom_bm25 | 0.342238 | 0.169025 | 0.144612 | 0.114086 | 0.271 | 0.256 |
| small3 df_le_0p5 | atom_bm25 | 0.371895 | 0.218614 | 0.185532 | 0.153290 | 0.348 | 0.343 |
| expansion5 m323 | atom_bm25 | 0.408496 | 0.114189 | 0.114547 | 0.083559 | 0.286 | 0.276 |
| expansion5 m326b | atom_bm25 | 0.408496 | 0.114189 | 0.114547 | 0.083559 | 0.286 | 0.276 |

## Scorer Rescue Rows

| Surface | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 | NDCG/dense | MAP/dense |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| small3 df_le_0p25 | df_le_0p25_m322_scorer | 0.649424 | 0.517923 | 0.426394 | 0.336861 | 0.800 | 0.755 |
| small3 df_le_0p5 | df_le_0p5_m322_scorer | 0.649910 | 0.512693 | 0.423318 | 0.331749 | 0.794 | 0.743 |
| expansion5 m323 | df_le_0p25_m322_scorer | 0.705396 | 0.351482 | 0.327870 | 0.233697 | 0.820 | 0.771 |
| expansion5 m326b | df_le_0p25_m322_scorer | 0.724188 | 0.365778 | 0.333950 | 0.242025 | 0.835 | 0.798 |

## All Compared Rows

| Surface | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 | NDCG/dense | MAP/dense |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| small3 df_le_0p25 | atom_bm25 | 0.342238 | 0.169025 | 0.144612 | 0.114086 | 0.271 | 0.256 |
| small3 df_le_0p25 | lexical_bm25 | 0.484541 | 0.392835 | 0.325236 | 0.258054 | 0.610 | 0.578 |
| small3 df_le_0p25 | unified_scale_0.5 | 0.537432 | 0.426231 | 0.354259 | 0.281426 | 0.665 | 0.630 |
| small3 df_le_0p25 | unified_scale_1 | 0.556274 | 0.433095 | 0.355543 | 0.283883 | 0.667 | 0.636 |
| small3 df_le_0p25 | df_le_0p25_m322_scorer | 0.649424 | 0.517923 | 0.426394 | 0.336861 | 0.800 | 0.755 |
| small3 df_le_0p25 | df_le_0p25_candidate_upper_bound | 0.722543 | 1.000000 | 0.842620 | 0.722543 | 1.581 | 1.619 |
| small3 df_le_0p5 | atom_bm25 | 0.371895 | 0.218614 | 0.185532 | 0.153290 | 0.348 | 0.343 |
| small3 df_le_0p5 | lexical_bm25 | 0.484541 | 0.392835 | 0.325236 | 0.258054 | 0.610 | 0.578 |
| small3 df_le_0p5 | unified_scale_0.5 | 0.545593 | 0.428679 | 0.356632 | 0.280254 | 0.669 | 0.628 |
| small3 df_le_0p5 | unified_scale_1 | 0.572548 | 0.439007 | 0.366062 | 0.290666 | 0.687 | 0.651 |
| small3 df_le_0p5 | df_le_0p5_m322_scorer | 0.649910 | 0.512693 | 0.423318 | 0.331749 | 0.794 | 0.743 |
| small3 df_le_0p5 | df_le_0p5_candidate_upper_bound | 0.722568 | 1.000000 | 0.841722 | 0.722568 | 1.579 | 1.619 |
| expansion5 m323 | atom_bm25 | 0.408496 | 0.114189 | 0.114547 | 0.083559 | 0.286 | 0.276 |
| expansion5 m323 | lexical_bm25 | 0.621390 | 0.296127 | 0.284080 | 0.206407 | 0.710 | 0.681 |
| expansion5 m323 | unified_scale_0.5 | 0.630520 | 0.293783 | 0.271892 | 0.199430 | 0.680 | 0.658 |
| expansion5 m323 | df_le_0p25_m322_scorer | 0.705396 | 0.351482 | 0.327870 | 0.233697 | 0.820 | 0.771 |
| expansion5 m323 | df_le_0p25_candidate_upper_bound | 0.768512 | 0.999004 | 0.841054 | 0.768512 | 2.102 | 2.535 |
| expansion5 m326b | atom_bm25 | 0.408496 | 0.114189 | 0.114547 | 0.083559 | 0.286 | 0.276 |
| expansion5 m326b | lexical_bm25 | 0.621390 | 0.296127 | 0.284080 | 0.206407 | 0.710 | 0.681 |
| expansion5 m326b | unified_scale_0.5 | 0.630520 | 0.293783 | 0.271892 | 0.199430 | 0.680 | 0.658 |
| expansion5 m326b | df_le_0p25_m322_scorer | 0.724188 | 0.365778 | 0.333950 | 0.242025 | 0.835 | 0.798 |
| expansion5 m326b | df_le_0p25_candidate_upper_bound | 0.768512 | 0.999004 | 0.841054 | 0.768512 | 2.102 | 2.535 |

## Interpretation

- Existing evidence is enough to say atom-only SAE ranking is not yet dense-faithful on the current M320/M323-style surfaces.
- The downstream scorer recovers a large amount of qrel quality, but that is not the same as proving the SAE representation itself preserves dense neighborhoods.
- Candidate upper bounds are high, so admission capacity exists; the missing piece is either representation faithfulness, ranking use of that representation, or both.

## Missing Evidence

- Per-query `exact dense top-k` versus `atom-only top-k` overlap on the current M320 checkpoint.
- Per-query overlap split by dataset and query family.
- The same audit on broad8 M344/M350 roots, not only small3/expansion5 aggregate artifacts.

## Next Step

Run a non-training export/eval that emits per-query atom-only rankings and compares them against exact dense teacher rankings. This can be done as a cache/export job after current training resources free up; it should not be mixed with BM25 or scorer rows.
