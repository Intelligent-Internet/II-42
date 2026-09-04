# ii42 M1123 Alpha Gate Separability Smoke

Verdict: `fail_no_stable_gate_signal`
- Train split: `train`
- Test split: `heldout`

| Transition | Samples | Classes | LODO AUC | LODO AP | Cross AUC | Cross AP | Transition verdict |
| --- | ---: | --- | ---: | ---: | ---: | ---: | --- |
| 0.50 -> 0.60 | 367 | clean_recall_gain:63, mixed_gain_damage:5, pure_top_damage:35, top_gain_no_recall:264 | 0.408579 | 0.880575 | 0.551216 | 0.502624 | `fail_no_stable_gate_signal` |
| 0.60 -> 0.75 | 381 | clean_recall_gain:65, mixed_gain_damage:7, pure_top_damage:35, top_gain_no_recall:274 | 0.447200 | 0.868972 | 0.445921 | 0.380228 | `fail_no_stable_gate_signal` |

## Features

`candidate_count`, `lex_atom_corr`, `lex_atom_overlap_10`, `lex_atom_overlap_50`, `lex_atom_overlap_100`, `lex_top1_atom_rank`, `atom_top1_lex_rank`, `lex_gap_top1_top10`, `atom_gap_top1_top10`
