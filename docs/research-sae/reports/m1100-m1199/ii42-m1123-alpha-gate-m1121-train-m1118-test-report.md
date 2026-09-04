# ii42 M1123 Alpha Gate Separability Smoke

Verdict: `fail_no_stable_gate_signal`

| Transition | Samples | Classes | LODO AUC | LODO AP | Cross AUC | Cross AP | Transition verdict |
| --- | ---: | --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | 70 | clean_recall_gain:8, mixed_gain_damage:1, pure_top_damage:37, top_gain_no_recall:24 | 0.594643 | 0.570425 | 0.552432 | 0.687383 | `fail_no_stable_gate_signal` |
| 0.25 -> 0.30 | 43 | clean_recall_gain:1, pure_top_damage:24, top_gain_no_recall:18 | 0.359226 | 0.537330 | 0.654902 | 0.688049 | `fail_no_stable_gate_signal` |
| 0.30 -> 0.50 | 63 | clean_recall_gain:4, pure_top_damage:41, top_gain_no_recall:18 | 0.569001 | 0.490219 | 0.517857 | 0.357620 | `fail_no_stable_gate_signal` |
| 0.50 -> 0.75 | 72 | clean_recall_gain:2, mixed_gain_damage:1, pure_top_damage:53, top_gain_no_recall:16 | 0.613001 | 0.482862 | 0.466667 | 0.225064 | `fail_no_stable_gate_signal` |

## Features

`candidate_count`, `lex_atom_corr`, `lex_atom_overlap_10`, `lex_atom_overlap_50`, `lex_atom_overlap_100`, `lex_top1_atom_rank`, `atom_top1_lex_rank`, `lex_gap_top1_top10`, `atom_gap_top1_top10`
