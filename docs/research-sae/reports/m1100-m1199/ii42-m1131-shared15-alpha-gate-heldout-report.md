# ii42 M1123 Alpha Gate Separability Smoke

Verdict: `possible_gate_signal`

| Transition | Samples | Classes | LODO AUC | LODO AP | Cross AUC | Cross AP | Transition verdict |
| --- | ---: | --- | ---: | ---: | ---: | ---: | --- |
| 0.50 -> 0.60 | 159 | clean_recall_gain:15, mixed_gain_damage:1, pure_top_damage:87, top_gain_no_recall:56 | 0.517249 | 0.615592 | 0.000000 | 0.000000 | `fail_no_stable_gate_signal` |
| 0.60 -> 0.75 | 177 | clean_recall_gain:14, mixed_gain_damage:2, pure_top_damage:104, top_gain_no_recall:57 | 0.655758 | 0.649069 | 0.000000 | 0.000000 | `possible_gate_signal` |

## Features

`candidate_count`, `lex_atom_corr`, `lex_atom_overlap_10`, `lex_atom_overlap_50`, `lex_atom_overlap_100`, `lex_top1_atom_rank`, `atom_top1_lex_rank`, `lex_gap_top1_top10`, `atom_gap_top1_top10`
