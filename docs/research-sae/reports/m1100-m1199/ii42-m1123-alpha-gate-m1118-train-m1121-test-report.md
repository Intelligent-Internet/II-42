# ii42 M1123 Alpha Gate Separability Smoke

Verdict: `fail_no_stable_gate_signal`

| Transition | Samples | Classes | LODO AUC | LODO AP | Cross AUC | Cross AP | Transition verdict |
| --- | ---: | --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | 62 | clean_recall_gain:10, pure_top_damage:25, top_gain_no_recall:27 | 0.562593 | 0.724434 | 0.558388 | 0.490424 | `fail_no_stable_gate_signal` |
| 0.25 -> 0.30 | 32 | pure_top_damage:15, top_gain_no_recall:17 | 0.612963 | 0.743115 | 0.539474 | 0.509459 | `fail_no_stable_gate_signal` |
| 0.30 -> 0.50 | 62 | clean_recall_gain:4, pure_top_damage:42, top_gain_no_recall:16 | 0.584880 | 0.543070 | 0.453437 | 0.353227 | `fail_no_stable_gate_signal` |
| 0.50 -> 0.75 | 64 | clean_recall_gain:2, pure_top_damage:49, top_gain_no_recall:13 | 0.370833 | 0.269146 | 0.473251 | 0.235091 | `fail_no_stable_gate_signal` |

## Features

`candidate_count`, `lex_atom_corr`, `lex_atom_overlap_10`, `lex_atom_overlap_50`, `lex_atom_overlap_100`, `lex_top1_atom_rank`, `atom_top1_lex_rank`, `lex_gap_top1_top10`, `atom_gap_top1_top10`
