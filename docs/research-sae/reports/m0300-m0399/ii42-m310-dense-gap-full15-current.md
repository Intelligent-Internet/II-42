# M310B Dense Gap Matrix

Ready datasets: 15 / 15
Partial datasets: 0
Upstream qrels/corpus gap datasets included in ready mean: 1

| Dataset | Status | Rows | BM25 R@100 | Dense R@100 | SAE R@100 | Best M310 | Best R@100 | Best NDCG@10 | Delta R@100 | Delta NDCG@10 |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: |
| nfcorpus | ready | 323 | 0.2564 | 0.3270 | 0.3045 | bm25_plus_latent_binary_bm25_additive_w0p5 | 0.3197 | 0.3419 | -0.0073 | -0.0160 |
| scifact | ready | 300 | 0.8859 | 0.9633 | 0.9577 | bm25_plus_latent_binary_bm25_additive_w0p35 | 0.9767 | 0.7910 | 0.0133 | 0.0431 |
| arguana | ready (upstream qrels/corpus gap) | 1401 | 0.9122 | 1.0000 | 0.9936 | bm25_plus_latent_binary_bm25_additive_w0p5 | 0.9936 | 0.4061 | -0.0064 | -0.0299 |
| scidocs | ready | 1000 | 0.3469 | 0.4958 | 0.4468 | bm25_plus_latent_binary_bm25_additive_w0p5 | 0.4565 | 0.2054 | -0.0393 | -0.0239 |
| fiqa | ready | 648 | 0.5086 | 0.8289 | 0.7428 | bm25_plus_latent_binary_bm25_additive_w0p35 | 0.7623 | 0.4465 | -0.0666 | -0.0700 |
| trec-covid | ready | 50 | 0.0972 | 0.1673 | 0.1388 | bm25_plus_latent_binary_bm25_additive_w0p35 | 0.1558 | 0.8500 | -0.0115 | -0.0629 |
| webis-touche2020 | ready | 49 | 0.5622 | 0.4928 | 0.4304 | bm25_plus_latent_binary_bm25_additive_w0p5 | 0.5552 | 0.3105 | 0.0624 | 0.0256 |
| cqadupstack | ready | 13145 | 0.5297 | 0.7825 | 0.6815 | bm25_plus_latent_binary_bm25_additive_w0p5 | 0.7140 | 0.3947 | -0.0685 | -0.0522 |
| quora | ready | 10000 | 0.9497 | 0.9960 | 0.9922 | bm25_plus_latent_binary_bm25_additive_w0p5 | 0.9955 | 0.8854 | -0.0005 | -0.0040 |
| nq | ready | 3452 | 0.6994 | 0.9610 | 0.8893 | bm25_plus_latent_binary_bm25_additive_w0p35 | 0.9235 | 0.5072 | -0.0375 | -0.0970 |
| dbpedia-entity | ready | 400 | 0.4059 | 0.5416 | 0.3610 | bm25_plus_latent_binary_bm25_additive_w0p5 | 0.4738 | 0.4148 | -0.0677 | -0.0638 |
| hotpotqa | ready | 7405 | 0.7474 | 0.8732 | 0.7325 | bm25_plus_latent_binary_bm25_additive_w0p5 | 0.8286 | 0.6784 | -0.0446 | -0.0667 |
| fever | ready | 6666 | 0.8512 | 0.9754 | 0.9539 | bm25_plus_latent_binary_bm25_additive_w0p35 | 0.9649 | 0.8470 | -0.0105 | -0.0573 |
| climate-fever | ready | 1535 | 0.3637 | 0.7074 | 0.5992 | bm25_plus_latent_binary_bm25_additive_w0p35 | 0.6230 | 0.3152 | -0.0844 | -0.0792 |
| msmarco | ready | 43 | 0.4242 | 0.5256 | 0.4301 | bm25_plus_latent_binary_bm25_additive_w0p35 | 0.5039 | 0.8218 | -0.0217 | -0.0412 |

## Ready Mean

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| dense | 0.7092 | 0.6670 | 0.5874 | 0.4318 |
| sae | 0.6436 | 0.5979 | 0.5122 | 0.3656 |
| posting_score | 0.6335 | 0.5576 | 0.4773 | 0.3374 |
| best_m310 | 0.6831 | 0.6276 | 0.5477 | 0.3973 |
| best_m310_minus_dense | -0.0260 | -0.0394 | -0.0397 | -0.0346 |
