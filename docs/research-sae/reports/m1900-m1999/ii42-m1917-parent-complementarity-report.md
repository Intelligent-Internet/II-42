# M1917 Parent Complementarity And PPLX Gate

The dense teacher is the frozen PPLX ranking already used by the P1
line. Qrels are used only to diagnose positive ranks, never to train
or select a checkpoint.

| Sparse route | Dense-only @100 | Sparse-only @100 | Pure PPLX |
| --- | ---: | ---: | --- |
| p1 | 6.5589% | 4.4106% | reject |
| m1914 | 7.0532% | 6.2167% | reject |
| opensearch | 6.9265% | 6.4385% | reject |

## Per-dataset Pair Counts

### fiqa

```json
{
  "dense_teacher_vs_m1914_at_100": {
    "both": 1023,
    "left_only": 321,
    "neither": 311,
    "right_only": 51
  },
  "dense_teacher_vs_m1914_at_1000": {
    "both": 1275,
    "left_only": 69,
    "neither": 184,
    "right_only": 178
  },
  "dense_teacher_vs_opensearch_at_100": {
    "both": 994,
    "left_only": 350,
    "neither": 312,
    "right_only": 50
  },
  "dense_teacher_vs_opensearch_at_1000": {
    "both": 1251,
    "left_only": 93,
    "neither": 194,
    "right_only": 168
  },
  "dense_teacher_vs_p1_at_100": {
    "both": 1216,
    "left_only": 128,
    "neither": 293,
    "right_only": 69
  },
  "dense_teacher_vs_p1_at_1000": {
    "both": 1342,
    "left_only": 2,
    "neither": 137,
    "right_only": 225
  },
  "m1914_vs_opensearch_at_100": {
    "both": 942,
    "left_only": 132,
    "neither": 530,
    "right_only": 102
  },
  "m1914_vs_opensearch_at_1000": {
    "both": 1363,
    "left_only": 90,
    "neither": 197,
    "right_only": 56
  },
  "m1914_vs_p1_at_100": {
    "both": 1003,
    "left_only": 71,
    "neither": 350,
    "right_only": 282
  },
  "m1914_vs_p1_at_1000": {
    "both": 1407,
    "left_only": 46,
    "neither": 93,
    "right_only": 160
  },
  "opensearch_vs_p1_at_100": {
    "both": 970,
    "left_only": 74,
    "neither": 347,
    "right_only": 315
  },
  "opensearch_vs_p1_at_1000": {
    "both": 1381,
    "left_only": 38,
    "neither": 101,
    "right_only": 186
  }
}
```

### arguana

```json
{
  "dense_teacher_vs_m1914_at_100": {
    "both": 1347,
    "left_only": 16,
    "right_only": 38
  },
  "dense_teacher_vs_m1914_at_1000": {
    "both": 1362,
    "left_only": 1,
    "right_only": 38
  },
  "dense_teacher_vs_opensearch_at_100": {
    "both": 1347,
    "left_only": 16,
    "right_only": 38
  },
  "dense_teacher_vs_opensearch_at_1000": {
    "both": 1362,
    "left_only": 1,
    "right_only": 38
  },
  "dense_teacher_vs_p1_at_100": {
    "both": 1350,
    "left_only": 13,
    "neither": 1,
    "right_only": 37
  },
  "dense_teacher_vs_p1_at_1000": {
    "both": 1362,
    "left_only": 1,
    "right_only": 38
  },
  "m1914_vs_opensearch_at_100": {
    "both": 1376,
    "left_only": 9,
    "neither": 7,
    "right_only": 9
  },
  "m1914_vs_opensearch_at_1000": {
    "both": 1399,
    "left_only": 1,
    "right_only": 1
  },
  "m1914_vs_p1_at_100": {
    "both": 1374,
    "left_only": 11,
    "neither": 3,
    "right_only": 13
  },
  "m1914_vs_p1_at_1000": {
    "both": 1399,
    "left_only": 1,
    "right_only": 1
  },
  "opensearch_vs_p1_at_100": {
    "both": 1373,
    "left_only": 12,
    "neither": 2,
    "right_only": 14
  },
  "opensearch_vs_p1_at_1000": {
    "both": 1399,
    "left_only": 1,
    "right_only": 1
  }
}
```

### nfcorpus

```json
{
  "dense_teacher_vs_m1914_at_100": {
    "both": 1514,
    "left_only": 768,
    "neither": 9186,
    "right_only": 866
  },
  "dense_teacher_vs_m1914_at_1000": {
    "both": 2115,
    "left_only": 167,
    "neither": 5951,
    "right_only": 4101
  },
  "dense_teacher_vs_opensearch_at_100": {
    "both": 1567,
    "left_only": 715,
    "neither": 9148,
    "right_only": 904
  },
  "dense_teacher_vs_opensearch_at_1000": {
    "both": 2153,
    "left_only": 129,
    "neither": 5893,
    "right_only": 4159
  },
  "dense_teacher_vs_p1_at_100": {
    "both": 1407,
    "left_only": 875,
    "neither": 9481,
    "right_only": 571
  },
  "dense_teacher_vs_p1_at_1000": {
    "both": 2144,
    "left_only": 138,
    "neither": 6282,
    "right_only": 3770
  },
  "m1914_vs_opensearch_at_100": {
    "both": 1915,
    "left_only": 465,
    "neither": 9398,
    "right_only": 556
  },
  "m1914_vs_opensearch_at_1000": {
    "both": 4881,
    "left_only": 1335,
    "neither": 4687,
    "right_only": 1431
  },
  "m1914_vs_p1_at_100": {
    "both": 1381,
    "left_only": 999,
    "neither": 9357,
    "right_only": 597
  },
  "m1914_vs_p1_at_1000": {
    "both": 4191,
    "left_only": 2025,
    "neither": 4395,
    "right_only": 1723
  },
  "opensearch_vs_p1_at_100": {
    "both": 1397,
    "left_only": 1074,
    "neither": 9282,
    "right_only": 581
  },
  "opensearch_vs_p1_at_1000": {
    "both": 4283,
    "left_only": 2029,
    "neither": 4391,
    "right_only": 1631
  }
}
```

### scifact

```json
{
  "dense_teacher_vs_m1914_at_100": {
    "both": 298,
    "left_only": 8,
    "neither": 7,
    "right_only": 26
  },
  "dense_teacher_vs_m1914_at_1000": {
    "both": 304,
    "left_only": 2,
    "right_only": 33
  },
  "dense_teacher_vs_opensearch_at_100": {
    "both": 294,
    "left_only": 12,
    "neither": 9,
    "right_only": 24
  },
  "dense_teacher_vs_opensearch_at_1000": {
    "both": 304,
    "left_only": 2,
    "right_only": 33
  },
  "dense_teacher_vs_p1_at_100": {
    "both": 287,
    "left_only": 19,
    "neither": 14,
    "right_only": 19
  },
  "dense_teacher_vs_p1_at_1000": {
    "both": 304,
    "left_only": 2,
    "neither": 2,
    "right_only": 31
  },
  "m1914_vs_opensearch_at_100": {
    "both": 314,
    "left_only": 10,
    "neither": 11,
    "right_only": 4
  },
  "m1914_vs_opensearch_at_1000": {
    "both": 336,
    "left_only": 1,
    "neither": 1,
    "right_only": 1
  },
  "m1914_vs_p1_at_100": {
    "both": 301,
    "left_only": 23,
    "neither": 10,
    "right_only": 5
  },
  "m1914_vs_p1_at_1000": {
    "both": 333,
    "left_only": 4,
    "right_only": 2
  },
  "opensearch_vs_p1_at_100": {
    "both": 299,
    "left_only": 19,
    "neither": 14,
    "right_only": 7
  },
  "opensearch_vs_p1_at_1000": {
    "both": 333,
    "left_only": 4,
    "right_only": 2
  }
}
```
