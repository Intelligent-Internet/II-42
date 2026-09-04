# SAE Generalization Matrix Report

Date: 2026-05-11

## Scope

This matrix applies one fixed recipe across BEIR-style sampled datasets. It is
stronger than tuning only on SciDocs, but it is still dataset-local document
SAE training, not zero-shot model transfer.

Datasets:

```text
scifact, scidocs, nfcorpus, arguana, fiqa
```

Runner:

```text
scripts/research_sae_generalization_matrix.py
```

Temporary full artifacts:

```text
/tmp/ii42_sae_generalization_matrix/generalization_matrix.json
/tmp/ii42_sae_generalization_matrix/generalization_matrix.md
```

## Aggregate By Variant

| Variant | Direct test R@100 | Direct test MRR@20 | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline_dcost10_mask999` | `0.7898` | `0.4928` | `0.9291` | `0.7642` | `1313.1` | `3146.6` |
| `q16_real_teacher` | `0.7621` | `0.4513` | `0.9028` | `0.7299` | `981.7` | `1714.1` |
| `q32_pseudo_frac10` | `0.7613` | `0.4449` | `0.8971` | `0.7148` | `1110.9` | `2317.0` |
| `q32_qsel010` | `0.7386` | `0.4228` | `0.8875` | `0.7146` | `901.7` | `1695.6` |
| `q64_pseudo_frac10` | `0.7686` | `0.4727` | `0.8991` | `0.7208` | `1176.0` | `2603.9` |
| `q64_qsel010` | `0.7508` | `0.4387` | `0.8941` | `0.7197` | `978.3` | `1907.8` |

## Dataset Results

| Dataset | Variant | Direct R@100 | Direct MRR@20 | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `baseline_dcost10_mask999` | `1.0000` | `0.5583` | `1.0000` | `0.8737` | `1075.5` | `1972.2` |
| `scifact` | `q16_real_teacher` | `1.0000` | `0.5664` | `1.0000` | `0.8603` | `955.9` | `1542.6` |
| `scifact` | `q32_pseudo_frac10` | `1.0000` | `0.4891` | `1.0000` | `0.8768` | `1151.3` | `2094.6` |
| `scifact` | `q32_qsel010` | `0.9667` | `0.5138` | `1.0000` | `0.8718` | `1009.5` | `1699.8` |
| `scifact` | `q64_pseudo_frac10` | `1.0000` | `0.5367` | `1.0000` | `0.8718` | `1209.2` | `2296.5` |
| `scifact` | `q64_qsel010` | `0.9667` | `0.5185` | `1.0000` | `0.8701` | `1056.7` | `1840.2` |
| `scidocs` | `baseline_dcost10_mask999` | `0.7133` | `0.4553` | `0.9180` | `0.7797` | `1023.1` | `1958.6` |
| `scidocs` | `q16_real_teacher` | `0.6867` | `0.4376` | `0.9160` | `0.7247` | `891.7` | `1481.9` |
| `scidocs` | `q32_pseudo_frac10` | `0.7000` | `0.5072` | `0.9120` | `0.6974` | `1214.7` | `2387.1` |
| `scidocs` | `q32_qsel010` | `0.6933` | `0.4486` | `0.9080` | `0.7128` | `922.9` | `1558.7` |
| `scidocs` | `q64_pseudo_frac10` | `0.6933` | `0.5254` | `0.9120` | `0.7008` | `1304.1` | `2756.3` |
| `scidocs` | `q64_qsel010` | `0.7000` | `0.4433` | `0.9080` | `0.7189` | `1015.4` | `1803.3` |
| `nfcorpus` | `baseline_dcost10_mask999` | `0.3506` | `0.4641` | `0.7526` | `0.8569` | `1974.5` | `5668.2` |
| `nfcorpus` | `q16_real_teacher` | `0.2486` | `0.3483` | `0.6407` | `0.7358` | `783.1` | `1042.0` |
| `nfcorpus` | `q32_pseudo_frac10` | `0.2298` | `0.2901` | `0.6011` | `0.6771` | `456.6` | `542.1` |
| `nfcorpus` | `q32_qsel010` | `0.1978` | `0.2355` | `0.5668` | `0.6776` | `250.2` | `276.6` |
| `nfcorpus` | `q64_pseudo_frac10` | `0.2562` | `0.3631` | `0.6210` | `0.6867` | `563.8` | `714.9` |
| `nfcorpus` | `q64_qsel010` | `0.2106` | `0.3047` | `0.5948` | `0.6897` | `407.1` | `497.8` |
| `arguana` | `baseline_dcost10_mask999` | `1.0000` | `0.4234` | `1.0000` | `0.5480` | `815.1` | `1532.5` |
| `arguana` | `q16_real_teacher` | `1.0000` | `0.4019` | `1.0000` | `0.5700` | `754.5` | `1369.1` |
| `arguana` | `q32_pseudo_frac10` | `1.0000` | `0.3804` | `1.0000` | `0.5747` | `976.2` | `1882.4` |
| `arguana` | `q32_qsel010` | `1.0000` | `0.3767` | `1.0000` | `0.5639` | `854.2` | `1587.5` |
| `arguana` | `q64_pseudo_frac10` | `1.0000` | `0.3748` | `1.0000` | `0.5747` | `996.5` | `1930.6` |
| `arguana` | `q64_qsel010` | `1.0000` | `0.3850` | `1.0000` | `0.5697` | `873.5` | `1628.7` |
| `fiqa` | `baseline_dcost10_mask999` | `0.8850` | `0.5629` | `0.9750` | `0.7628` | `1677.2` | `4601.4` |
| `fiqa` | `q16_real_teacher` | `0.8750` | `0.5023` | `0.9575` | `0.7588` | `1523.2` | `3134.9` |
| `fiqa` | `q32_pseudo_frac10` | `0.8767` | `0.5578` | `0.9725` | `0.7482` | `1755.8` | `4679.1` |
| `fiqa` | `q32_qsel010` | `0.8350` | `0.5392` | `0.9625` | `0.7470` | `1471.9` | `3355.6` |
| `fiqa` | `q64_pseudo_frac10` | `0.8933` | `0.5636` | `0.9625` | `0.7699` | `1806.7` | `5321.3` |
| `fiqa` | `q64_qsel010` | `0.8767` | `0.5418` | `0.9675` | `0.7502` | `1539.1` | `3769.0` |

## Conclusion

The baseline document SAE remains the best aggregate quality point. Query-side
cost controls are useful because they reduce postings and candidate coverage,
but the current normalized-DF signal is too blunt. It reduces physical cost by
suppressing common latents, not by estimating the marginal new candidates a
latent opens for the current query.

The next useful direction is an incremental candidate-budget query gate, not
another fixed fusion-weight or pseudo-query fraction sweep.
