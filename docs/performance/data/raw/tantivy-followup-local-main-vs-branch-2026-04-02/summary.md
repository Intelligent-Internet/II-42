# Local Main vs Branch Comparison

## psql_bm25s_ids

- query delta median: `+18.60%` (wins `14`, losses `1`)
- build delta median: `+2.69%`

## psql_bm25s_text

- query delta median: `+24.52%` (wins `13`, losses `2`)
- build delta median: `+2.16%`

| Dataset | Path | Main QPS | Branch QPS | QPS delta | Main build ms | Branch build ms | Build delta |
|---|---:|---:|---:|---:|---:|---:|---:|
| arguana | psql_bm25s_ids | 1460.40 | 1355.54 | -7.18% | 80.52 | 82.69 | +2.69% |
| arguana | psql_bm25s_text | 1174.01 | 738.01 | -37.14% | 135.74 | 118.39 | -12.79% |
| climate-fever | psql_bm25s_ids | 78.62 | 83.28 | +5.92% | 54185.46 | 53957.96 | -0.42% |
| climate-fever | psql_bm25s_text | 62.03 | 74.88 | +20.70% | 73673.39 | 78026.12 | +5.91% |
| cqadupstack | psql_bm25s_ids | 464.34 | 608.51 | +31.05% | 2843.40 | 2880.68 | +1.31% |
| cqadupstack | psql_bm25s_text | 444.51 | 602.82 | +35.62% | 5502.71 | 5080.32 | -7.68% |
| dbpedia-entity | psql_bm25s_ids | 200.34 | 247.85 | +23.71% | 20865.11 | 22632.00 | +8.47% |
| dbpedia-entity | psql_bm25s_text | 126.28 | 158.96 | +25.88% | 40224.49 | 33145.00 | -17.60% |
| fever | psql_bm25s_ids | 167.28 | 186.38 | +11.42% | 58617.34 | 52716.36 | -10.07% |
| fever | psql_bm25s_text | 165.74 | 185.49 | +11.91% | 83040.57 | 83667.07 | +0.75% |
| fiqa | psql_bm25s_ids | 1166.79 | 1659.17 | +42.20% | 354.48 | 359.50 | +1.41% |
| fiqa | psql_bm25s_text | 1105.27 | 1376.32 | +24.52% | 617.48 | 622.32 | +0.78% |
| hotpotqa | psql_bm25s_ids | 100.70 | 109.87 | +9.10% | 24933.43 | 24582.58 | -1.41% |
| hotpotqa | psql_bm25s_text | 100.71 | 109.26 | +8.49% | 33345.49 | 34065.27 | +2.16% |
| msmarco | psql_bm25s_ids | 130.73 | 145.37 | +11.19% | 63021.88 | 59676.35 | -5.31% |
| msmarco | psql_bm25s_text | 129.29 | 144.02 | +11.40% | 79982.52 | 94468.21 | +18.11% |
| nfcorpus | psql_bm25s_ids | 5323.40 | 6114.49 | +14.86% | 66.44 | 74.69 | +12.41% |
| nfcorpus | psql_bm25s_text | 4390.88 | 5865.52 | +33.58% | 89.72 | 99.25 | +10.63% |
| nq | psql_bm25s_ids | 231.07 | 274.05 | +18.60% | 22599.82 | 20585.73 | -8.91% |
| nq | psql_bm25s_text | 213.29 | 287.68 | +34.88% | 30678.79 | 29955.63 | -2.36% |
| quora | psql_bm25s_ids | 552.03 | 918.38 | +66.36% | 333.84 | 353.63 | +5.93% |
| quora | psql_bm25s_text | 568.84 | 947.55 | +66.58% | 605.80 | 633.78 | +4.62% |
| scidocs | psql_bm25s_ids | 1603.82 | 2053.14 | +28.02% | 209.67 | 220.29 | +5.06% |
| scidocs | psql_bm25s_text | 1396.22 | 1824.41 | +30.67% | 365.53 | 371.78 | +1.71% |
| scifact | psql_bm25s_ids | 3151.40 | 3954.39 | +25.48% | 67.41 | 79.85 | +18.46% |
| scifact | psql_bm25s_text | 2768.86 | 3592.02 | +29.73% | 115.65 | 119.73 | +3.53% |
| trec-covid | psql_bm25s_ids | 381.07 | 487.59 | +27.95% | 1250.15 | 1326.87 | +6.14% |
| trec-covid | psql_bm25s_text | 275.19 | 205.14 | -25.46% | 2293.65 | 4133.49 | +80.21% |
| webis-touche2020 | psql_bm25s_ids | 197.30 | 209.10 | +5.98% | 5054.63 | 5322.60 | +5.30% |
| webis-touche2020 | psql_bm25s_text | 128.88 | 143.34 | +11.22% | 8912.30 | 9649.01 | +8.27% |
