# M1963 Raw M190 Exact-BMP Full15 Report

Decision: **reject_m190_quality_fallback**

Quality decision: **reject_m190_quality_fallback**

## Scope

- completed datasets: `15/15`;
- official queries: `46417`;
- representation: frozen raw M190 learned impacts;
- execution: exact patched BMP, u8 postings and u32 accumulation;
- no BM25 fusion, pruning, retraining, or dataset-specific tuning;
- qrels are evaluation-only; corpora were visible during M190 training.
- MSMARCO embeddings: Betty-recovered historical M150 PPLX artifact;
- MSMARCO cross-hardware rematerialization: `False`;
- MSMARCO PostgreSQL halfvec export: `False`;
- MSMARCO document SHA-256: `fa6e515b03add6cff23d3e4f19c91b7061f5b805dfeefaabbad94057dca9c4ba`.

## Macro Matrix

| Method | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.374297 | 0.274610 | 0.562964 | 0.475641 | 0.735619 |
| PPLX Dense | 0.544873 | 0.412036 | 0.670880 | 0.650711 | 0.810525 |
| P2.1 | 0.490809 | 0.371455 | 0.666885 | 0.595950 | 0.839658 |
| Raw M190 Exact BMP | 0.492105 | 0.365484 | 0.643985 | 0.598575 | 0.812235 |

## Per-Dataset Matrix

### Recall@100

| Dataset | BM25 | PPLX Dense | P2.1 | Raw M190 Exact BMP |
| --- | ---: | ---: | ---: | ---: |
| arguana | 0.952891 | 0.972877 | 0.987866 | 0.991435 |
| climate-fever | 0.341694 | 0.703507 | 0.530945 | 0.600033 |
| cqadupstack | 0.524878 | 0.747247 | 0.730500 | 0.681649 |
| dbpedia-entity | 0.390971 | 0.458798 | 0.528869 | 0.361331 |
| fever | 0.838526 | 0.905383 | 0.949535 | 0.953861 |
| fiqa | 0.510764 | 0.804102 | 0.662176 | 0.744710 |
| hotpotqa | 0.724646 | 0.798447 | 0.816205 | 0.732546 |
| msmarco | 0.408121 | 0.472836 | 0.487246 | 0.430354 |
| nfcorpus | 0.233236 | 0.269111 | 0.299101 | 0.305054 |
| nq | 0.678568 | 0.912732 | 0.919298 | 0.888833 |
| quora | 0.947661 | 0.984258 | 0.988816 | 0.992132 |
| scidocs | 0.348567 | 0.484483 | 0.467100 | 0.446483 |
| scifact | 0.882556 | 0.898222 | 0.956000 | 0.957667 |
| trec-covid | 0.100074 | 0.157343 | 0.161646 | 0.138998 |
| webis-touche2020 | 0.561303 | 0.493851 | 0.517975 | 0.434697 |

### MAP@100

| Dataset | BM25 | PPLX Dense | P2.1 | Raw M190 Exact BMP |
| --- | ---: | ---: | ---: | ---: |
| arguana | 0.237186 | 0.300000 | 0.286486 | 0.277723 |
| climate-fever | 0.097173 | 0.314161 | 0.175821 | 0.253279 |
| cqadupstack | 0.265961 | 0.389596 | 0.380379 | 0.311445 |
| dbpedia-entity | 0.173662 | 0.250729 | 0.261121 | 0.183605 |
| fever | 0.395967 | 0.820341 | 0.713127 | 0.782054 |
| fiqa | 0.183892 | 0.451725 | 0.297809 | 0.375032 |
| hotpotqa | 0.430958 | 0.619042 | 0.575048 | 0.480362 |
| msmarco | 0.264019 | 0.383274 | 0.346804 | 0.319736 |
| nfcorpus | 0.137331 | 0.139780 | 0.169076 | 0.140234 |
| nq | 0.203360 | 0.517974 | 0.415296 | 0.425078 |
| quora | 0.695173 | 0.851136 | 0.807626 | 0.834403 |
| scidocs | 0.103194 | 0.159333 | 0.142215 | 0.136966 |
| scifact | 0.626833 | 0.685142 | 0.679675 | 0.726377 |
| trec-covid | 0.066611 | 0.131466 | 0.131082 | 0.105185 |
| webis-touche2020 | 0.237831 | 0.166837 | 0.190253 | 0.130782 |

### NDCG@10

| Dataset | BM25 | PPLX Dense | P2.1 | Raw M190 Exact BMP |
| --- | ---: | ---: | ---: | ---: |
| arguana | 0.344115 | 0.429182 | 0.411456 | 0.396207 |
| climate-fever | 0.127530 | 0.394597 | 0.228576 | 0.324429 |
| cqadupstack | 0.291987 | 0.431176 | 0.418136 | 0.349300 |
| dbpedia-entity | 0.239804 | 0.387310 | 0.353671 | 0.300825 |
| fever | 0.449661 | 0.846450 | 0.766722 | 0.823458 |
| fiqa | 0.231073 | 0.510599 | 0.358516 | 0.429933 |
| hotpotqa | 0.511627 | 0.688827 | 0.659351 | 0.559824 |
| msmarco | 0.366717 | 0.647114 | 0.524892 | 0.624953 |
| nfcorpus | 0.306793 | 0.322832 | 0.347391 | 0.321274 |
| nq | 0.242799 | 0.584749 | 0.481829 | 0.486072 |
| quora | 0.738212 | 0.880790 | 0.844995 | 0.866196 |
| scidocs | 0.150534 | 0.225314 | 0.200236 | 0.196089 |
| scifact | 0.663931 | 0.714060 | 0.723722 | 0.766418 |
| trec-covid | 0.571959 | 0.833468 | 0.752852 | 0.721827 |
| webis-touche2020 | 0.377718 | 0.276631 | 0.289785 | 0.214767 |

### MRR@20

| Dataset | BM25 | PPLX Dense | P2.1 | Raw M190 Exact BMP |
| --- | ---: | ---: | ---: | ---: |
| arguana | 0.233893 | 0.298787 | 0.284977 | 0.275750 |
| climate-fever | 0.181684 | 0.513265 | 0.315448 | 0.436738 |
| cqadupstack | 0.295992 | 0.425859 | 0.416496 | 0.344689 |
| dbpedia-entity | 0.504366 | 0.733080 | 0.689000 | 0.623931 |
| fever | 0.410674 | 0.862403 | 0.756337 | 0.827071 |
| fiqa | 0.292597 | 0.599713 | 0.439020 | 0.514801 |
| hotpotqa | 0.662780 | 0.834553 | 0.824656 | 0.717353 |
| msmarco | 0.743909 | 0.961240 | 0.884367 | 0.937984 |
| nfcorpus | 0.515219 | 0.548789 | 0.567236 | 0.529838 |
| nq | 0.215921 | 0.541040 | 0.435048 | 0.446183 |
| quora | 0.735647 | 0.874570 | 0.838923 | 0.859657 |
| scidocs | 0.277201 | 0.385142 | 0.348686 | 0.347982 |
| scifact | 0.634838 | 0.694454 | 0.688295 | 0.741064 |
| trec-covid | 0.817222 | 0.970000 | 0.930000 | 0.936667 |
| webis-touche2020 | 0.612666 | 0.517772 | 0.520760 | 0.438916 |

### Candidate Upper Bound@1000

| Dataset | BM25 | PPLX Dense | P2.1 | Raw M190 Exact BMP |
| --- | ---: | ---: | ---: | ---: |
| arguana | 0.990007 | 0.972877 | 1.000000 | 1.000000 |
| climate-fever | 0.567427 | 0.836721 | 0.743779 | 0.763941 |
| cqadupstack | 0.697639 | 0.870515 | 0.888289 | 0.867159 |
| dbpedia-entity | 0.580338 | 0.629907 | 0.733407 | 0.584174 |
| fever | 0.933235 | 0.912997 | 0.967121 | 0.971156 |
| fiqa | 0.725380 | 0.912474 | 0.855207 | 0.929293 |
| hotpotqa | 0.847738 | 0.856516 | 0.901283 | 0.849021 |
| msmarco | 0.672591 | 0.700965 | 0.784065 | 0.671671 |
| nfcorpus | 0.424066 | 0.541961 | 0.588587 | 0.607768 |
| nq | 0.860926 | 0.935762 | 0.980229 | 0.963355 |
| quora | 0.986610 | 0.987865 | 0.997925 | 0.999245 |
| scidocs | 0.561417 | 0.753817 | 0.734433 | 0.730600 |
| scifact | 0.965000 | 0.918222 | 0.993333 | 0.996667 |
| trec-covid | 0.362988 | 0.498362 | 0.573787 | 0.464442 |
| webis-touche2020 | 0.858916 | 0.828912 | 0.853424 | 0.785025 |

## Engine Rows

| Dataset | Gate | Docs | Bytes/doc | Build s | Peak RSS GiB | p50 ms | p95 ms | Sample Recall retention | Exact boundary |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| arguana | True | 8674 | 881.9 | 0.261 | 0.13 | 0.668 | 0.861 | 1.000000 | 1.000000 |
| climate-fever | True | 5416593 | 1304.9 | 209.965 | 59.02 | 7.088 | 27.290 | 1.000000 | 1.000000 |
| cqadupstack | True | 457199 | 1164.3 | 13.622 | 4.28 | 11.334 | 25.923 | 1.000000 | 1.000000 |
| dbpedia-entity | True | 4635922 | 1279.0 | 162.831 | 43.23 | 13.906 | 43.729 | 1.000000 | 1.000000 |
| fever | False | 5416568 | 1296.9 | 148.649 | 58.30 | 19.104 | 60.274 | 1.000000 | 1.000000 |
| fiqa | True | 57638 | 1200.1 | 1.522 | 0.65 | 5.713 | 7.297 | 1.017094 | 1.000000 |
| hotpotqa | False | 5233329 | 1245.7 | 164.038 | 46.24 | 28.159 | 82.004 | 1.000000 | 1.000000 |
| msmarco | True | 8841823 | 938.8 | 236.834 | 60.01 | 8.899 | 18.420 | 1.001180 | 1.000000 |
| nfcorpus | True | 3633 | 1008.3 | 0.117 | 0.08 | 0.266 | 0.346 | 1.000000 | 1.000000 |
| nq | True | 2681468 | 835.7 | 69.166 | 17.85 | 5.096 | 10.544 | 1.000000 | 1.000000 |
| quora | True | 522931 | 1163.0 | 14.378 | 4.34 | 5.841 | 19.987 | 1.000000 | 1.000000 |
| scidocs | True | 25657 | 1249.4 | 0.697 | 0.32 | 2.826 | 3.234 | 1.000000 | 1.000000 |
| scifact | True | 5183 | 1237.8 | 0.157 | 0.10 | 0.699 | 0.780 | 1.000000 | 1.000000 |
| trec-covid | True | 171331 | 1136.9 | 4.811 | 1.67 | 17.575 | 20.132 | 0.997422 | 1.000000 |
| webis-touche2020 | True | 382545 | 861.7 | 11.076 | 2.74 | 3.010 | 13.146 | 1.000000 | 1.000000 |

## Quality Attribution

Pearson correlations compare the M190-minus-P2.1 CUB delta with
the corresponding principal-metric delta across dataset rows:

- Recall@100: `0.848208`;
- MAP@100: `0.686959`;
- NDCG@10: `0.371080`;
- MRR@20: `0.442869`.

Rows with MAP delta below `-0.03`: `cqadupstack, dbpedia-entity, hotpotqa, webis-touche2020`. `4`/`4` also have lower CUB than P2.1.

This is descriptive rather than causal evidence. A strong positive
association and lower CUB on every material-loss row identify
candidate coverage as the first bottleneck; they do not prove that
ranking calibration is irrelevant on smaller-loss rows.

## Gates

- full15 complete: `True`;
- all native engine gates pass: `False`;
- dense Recall retention: `0.959912`;
- dense MAP retention: `0.887020`;
- principal macro wins vs P2.1: `2/4`.

Promotion requires all 15 signed rows, every engine gate, and
both dense-retention floors.
The fixed M190+BM25 broad9 hybrid remains a separate-candidate
quality ceiling and is not represented as a single-index product.

## Conclusion

The full15 quality surface rejects raw M190 as a deployable
self-trained fallback. Dense Recall retention passes, but dense
MAP retention misses its floor. The small NDCG/MRR gains over
P2.1 do not offset the MAP and Recall losses. Freeze the audited
checkpoint as training evidence and stop historical-route
optimization; do not reopen scorer, DF-pruning, or checkpoint
tuning on this route.
