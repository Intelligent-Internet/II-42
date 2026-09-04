# SAE M84 Label-Aware Stage-A Closure Plan

Status: planned. M83 introduced balanced sampling and a closure-aware
checkpoint gate. Early M83 evidence shows that sampling alone repeats the M82
failure mode: Recall can be protected, but MRR/top-rank order still carries too
much sparse tax.

## Rationale

Stage A is still a sparse-preservation task, not a product ranking task. The
primary target remains dense-student behavior. However, the evaluation surface is
based on labeled positives, and M82 showed that pure dense-distribution KL can
leave relevant documents just below the top ranks. This hurts MRR more than
Recall.

M84 therefore adds an optional, low-weight multi-positive CE term:

```text
loss =
    reconstruction
  + dense teacher KL
  + pairwise teacher top-rank loss
  + small label-positive CE
```

The CE term is intentionally small. It should only anchor known positive
documents when the dense teacher and sparse student are close; it must not
overwrite the dense teacher distribution.

## First Matrix

| Run | Init | KL | Pairwise | Label CE | Recon | Text records |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `m84-ce010-kl4-pair05-recon025-text120k` | fresh | 4.0 | 0.5 | 0.10 | 0.25 | 120k |
| `m84-ce020-kl4-pair05-recon025-text120k` | fresh | 4.0 | 0.5 | 0.20 | 0.25 | 120k |
| `m84-ce010-kl3-pair04-recon035-text120k` | fresh | 3.0 | 0.4 | 0.10 | 0.35 | 120k |
| `m84-finetune-m82best-ce005-kl2-pair02` | M82 best | 2.0 | 0.2 | 0.05 | 0.25 | existing |

## Gate

M84 uses the same Stage-A closure target as M83:

- overall Recall@10 tax `>= -0.006`;
- overall MRR tax `>= -0.020`;
- overall NDCG@10 tax `>= -0.015`;
- every source-family NDCG@10 tax `>= -0.020`;
- no source-family Recall@10 tax below `-0.010`.

If M84 still fails, the remaining blocker is likely not scalar objective
pressure. The next move should be a representation change: more active atoms,
different value transform, or a separate positive-coverage head before Stage B.
