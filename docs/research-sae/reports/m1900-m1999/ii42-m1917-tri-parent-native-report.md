# M1917 Tri-Parent Common Native Matrix

Decision: **no_single_parent_tri_parent_frontier**

All sparse routes use the same exact PostgreSQL normalized-postings
backend and full official rows. This four-row result selects the next
research stage; it is not a complete BEIR15 promotion.

## Per-dataset Quality

| Dataset | Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fiqa | p1 | 0.463089 | 0.401005 | 0.770400 | 0.552099 | 0.544830 | 0.922898 |
| fiqa | m1914 | 0.358606 | 0.300585 | 0.665214 | 0.444843 | 0.350833 | 0.860618 |
| fiqa | opensearch | 0.370244 | 0.310860 | 0.656042 | 0.456006 | 0.353827 | 0.849637 |
| arguana | p1 | 0.398682 | 0.276755 | 0.990007 | 0.274830 | 0.610043 | 0.999286 |
| arguana | m1914 | 0.404141 | 0.282061 | 0.988580 | 0.280458 | 0.535675 | 0.999286 |
| arguana | opensearch | 0.397538 | 0.277422 | 0.988580 | 0.275082 | 0.527645 | 0.999286 |
| nfcorpus | p1 | 0.272408 | 0.118020 | 0.266389 | 0.465003 | 0.380743 | 0.558879 |
| nfcorpus | m1914 | 0.339391 | 0.164808 | 0.292702 | 0.559041 | 0.318266 | 0.587925 |
| nfcorpus | opensearch | 0.344921 | 0.160261 | 0.285311 | 0.566408 | 0.330929 | 0.573762 |
| scifact | p1 | 0.643097 | 0.614619 | 0.897667 | 0.623941 | 0.438000 | 0.986667 |
| scifact | m1914 | 0.708670 | 0.667947 | 0.954333 | 0.678083 | 0.401767 | 0.993333 |
| scifact | opensearch | 0.708953 | 0.675193 | 0.934333 | 0.685412 | 0.404167 | 0.993333 |

## Macro And Native Cost

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB | Storage MiB | p95 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| p1 | 0.444319 | 0.352600 | 0.731116 | 0.478968 | 0.493404 | 0.866933 | 703.59 | 108.038 |
| m1914 | 0.452702 | 0.353850 | 0.725207 | 0.490606 | 0.401635 | 0.860291 | 1026.16 | 90.848 |
| opensearch | 0.455414 | 0.355934 | 0.716066 | 0.495727 | 0.404142 | 0.854005 | 1276.27 | 176.284 |

Product frontier: `p1, m1914, opensearch`.

## Severe Row Harm

```json
{
  "m1914_minus_opensearch": [],
  "m1914_minus_p1": [
    "fiqa"
  ],
  "opensearch_minus_m1914": [
    "scifact"
  ],
  "opensearch_minus_p1": [
    "fiqa"
  ],
  "p1_minus_m1914": [
    "nfcorpus",
    "scifact"
  ],
  "p1_minus_opensearch": [
    "nfcorpus",
    "scifact"
  ]
}
```
