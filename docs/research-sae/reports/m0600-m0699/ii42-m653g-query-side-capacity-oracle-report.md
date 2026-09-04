# M653G Query-Side Capacity Oracle

Status: `oracle_capacity_exists`

This is not a deployable model.  It is a per-query oracle that fits
dense scores inside the frozen P1 query support and frozen P1 document
posting space.  Qrels are used only for reporting.

## Key Conclusion

The first-stage problem is not mathematically blocked by frozen P1 document
postings or original query support.  On shared15, every tested ridge oracle
passes the dense-equivalence gate.  The conservative `ridge_10` oracle keeps
the fitted query extremely close to P1 (`query_p1_cosine=0.999874`,
`query_delta_l2=0.019906`) while improving `O@100` by `+0.004307`,
`O@256` by `+0.003531`, `CUB` by `+0.000061`, and `Recall@100` by
`+0.000303`.

This means M653F failed because the current trainable boundary/listwise
compiler does not learn the safe per-query dense-score fit, not because
query-side dense-equivalence is impossible.  The next trainable version should
distill the ridge teacher directly, then replay the learned compiler through
the same shared15 gate.

## Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 | Query cos | Delta L2 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.960670 | 1.000000 | 1.000000 | 0.788058 | 0.694729 | 0.853308 | 0.872179 | n/a | n/a |
| `p1_native` | 0.960323 | 0.943737 | 0.947923 | 0.786182 | 0.693945 | 0.851777 | 0.871168 | n/a | n/a |
| `ridge_0.001` | 0.960345 | 0.998545 | 0.961376 | 0.787585 | 0.694428 | 0.853268 | 0.871806 | 0.949909 | 0.332790 |
| `ridge_0.01` | 0.960846 | 0.992004 | 0.968623 | 0.787495 | 0.694487 | 0.853130 | 0.872148 | 0.969797 | 0.256199 |
| `ridge_0.1` | 0.960793 | 0.977520 | 0.971223 | 0.786837 | 0.693890 | 0.853028 | 0.871177 | 0.991660 | 0.133942 |
| `ridge_1` | 0.960430 | 0.960438 | 0.961222 | 0.786877 | 0.694308 | 0.852774 | 0.871217 | 0.998904 | 0.051305 |
| `ridge_10` | 0.960383 | 0.948044 | 0.951454 | 0.786169 | 0.693869 | 0.852080 | 0.870559 | 0.999874 | 0.019906 |

## Decision

```json
{
  "candidates": [
    {
      "deltas": {
        "accuracy": 0.0,
        "candidate_upper_bound": 2.2155233051268652e-05,
        "dense_overlap_at_10": 0.06379620313241552,
        "dense_overlap_at_100": 0.05480851763961414,
        "dense_overlap_at_256": 0.013452734436797953,
        "dense_overlap_at_50": 0.056326922955228675,
        "hit_rate_at_10": 0.0006666666666665932,
        "hit_rate_at_100": 0.0,
        "hit_rate_at_1000": 0.0,
        "hit_rate_at_20": -0.0013333333333332975,
        "map_at_10": 0.0005498495204111498,
        "map_at_100": 0.0004828015459860113,
        "map_at_1000": 0.0003239304332249837,
        "map_at_20": 6.74149187600559e-05,
        "mrr_at_10": 0.0008042328042329183,
        "mrr_at_100": 0.00070115871746812,
        "mrr_at_1000": 0.0007013918575278222,
        "mrr_at_20": 0.0006371513276313134,
        "ndcg_at_10": 0.0014029337882688342,
        "ndcg_at_100": 0.0008818976202769058,
        "ndcg_at_1000": 0.00039117397262278253,
        "ndcg_at_20": -8.368279678738766e-05,
        "precision_at_10": 0.0001306122448980207,
        "precision_at_100": 3.782945736435783e-05,
        "precision_at_1000": -9.800000000000086e-05,
        "precision_at_20": -0.0006496282233824924,
        "query_delta_l2": 0.3327902954106909,
        "query_p1_cosine": 0.9499085460262686,
        "recall_at_10": 0.0019298709786954582,
        "recall_at_100": 0.0014916782348709523,
        "recall_at_1000": 2.2155233051268652e-05,
        "recall_at_20": -0.0019245168708023064,
        "support_size": 512.0
      },
      "passed_dense_equivalence": true,
      "source": "ridge_0.001"
    },
    {
      "deltas": {
        "accuracy": 0.0,
        "candidate_upper_bound": 0.0005234547179941984,
        "dense_overlap_at_10": 0.054389590254706466,
        "dense_overlap_at_100": 0.048267062173706576,
        "dense_overlap_at_256": 0.020699476447555543,
        "dense_overlap_at_50": 0.049201075779148895,
        "hit_rate_at_10": 0.0006666666666665932,
        "hit_rate_at_100": 0.0,
        "hit_rate_at_1000": 0.0,
        "hit_rate_at_20": -0.0013333333333332975,
        "map_at_10": 0.0005714258516138493,
        "map_at_100": 0.0005413672554253068,
        "map_at_1000": 0.00046047111003599817,
        "map_at_20": 0.00011995850088131199,
        "mrr_at_10": 0.0011428571428573342,
        "mrr_at_100": 0.0010411147228249762,
        "mrr_at_1000": 0.001040980275975767,
        "mrr_at_20": 0.0009794004728216343,
        "ndcg_at_10": 0.0013127374537029723,
        "ndcg_at_100": 0.0009147899173901353,
        "ndcg_at_1000": 0.0006474431965955985,
        "ndcg_at_20": -1.2505565749276748e-05,
        "precision_at_10": 0.0003306122448980542,
        "precision_at_100": 2.449612403099266e-05,
        "precision_at_1000": -4.7333333333336725e-05,
        "precision_at_20": -0.0005054421768707629,
        "query_delta_l2": 0.25619920760180503,
        "query_p1_cosine": 0.9697968183652904,
        "recall_at_10": 0.001158029439806696,
        "recall_at_100": 0.0013533628349893245,
        "recall_at_1000": 0.0005234547179941984,
        "recall_at_20": -0.0019761813534692196,
        "support_size": 512.0
      },
      "passed_dense_equivalence": true,
      "source": "ridge_0.01"
    },
    {
      "deltas": {
        "accuracy": 0.0,
        "candidate_upper_bound": 0.0004698011589533424,
        "dense_overlap_at_10": 0.034466286979908056,
        "dense_overlap_at_100": 0.03378336971998097,
        "dense_overlap_at_256": 0.02330007860702399,
        "dense_overlap_at_50": 0.03357103622844482,
        "hit_rate_at_10": -0.0006666666666667043,
        "hit_rate_at_100": 0.0,
        "hit_rate_at_1000": 0.0,
        "hit_rate_at_20": -0.0013333333333332975,
        "map_at_10": -0.00016492981695193087,
        "map_at_100": -5.5304730275596015e-05,
        "map_at_1000": -6.083166555281849e-05,
        "map_at_20": -0.0003412478323180368,
        "mrr_at_10": 3.730158730175237e-05,
        "mrr_at_100": 7.08049800549615e-05,
        "mrr_at_1000": 7.014615253009282e-05,
        "mrr_at_20": 8.347208347192492e-06,
        "ndcg_at_10": 0.0006545885563304932,
        "ndcg_at_100": 0.0003541886164952768,
        "ndcg_at_1000": 0.00020486013523968172,
        "ndcg_at_20": -0.00033713501456789086,
        "precision_at_10": 0.00019727891156473554,
        "precision_at_100": -2.170542635682171e-06,
        "precision_at_1000": -7.3333333333314155e-06,
        "precision_at_20": -0.00016802721088438055,
        "query_delta_l2": 0.13394153712437867,
        "query_p1_cosine": 0.9916601441657878,
        "recall_at_10": 0.0006430726886306193,
        "recall_at_100": 0.001250975219605821,
        "recall_at_1000": 0.0004698011589533424,
        "recall_at_20": -0.0018847763850029509,
        "support_size": 512.0
      },
      "passed_dense_equivalence": true,
      "source": "ridge_0.1"
    },
    {
      "deltas": {
        "accuracy": 0.0,
        "candidate_upper_bound": 0.00010753099075111461,
        "dense_overlap_at_10": 0.016560480936560595,
        "dense_overlap_at_100": 0.01670121816168313,
        "dense_overlap_at_256": 0.013299076243869501,
        "dense_overlap_at_50": 0.01647626008542935,
        "hit_rate_at_10": 0.0013333333333332975,
        "hit_rate_at_100": 0.0,
        "hit_rate_at_1000": 0.0,
        "hit_rate_at_20": -0.0013333333333332975,
        "map_at_10": 0.00021259776405613273,
        "map_at_100": 0.0003632282665694264,
        "map_at_1000": 0.00039554880578762663,
        "map_at_20": 0.00018076442208969645,
        "mrr_at_10": 0.0002671957671956937,
        "mrr_at_100": 0.00010404845463607426,
        "mrr_at_1000": 0.00010313786640370193,
        "mrr_at_20": 4.849636947168445e-05,
        "ndcg_at_10": 0.0006947297026870247,
        "ndcg_at_100": 0.00036710813007423404,
        "ndcg_at_1000": 0.00024329372630926827,
        "ndcg_at_20": 0.00010088199427005229,
        "precision_at_10": -6.666666666660381e-05,
        "precision_at_100": 8.259769023891139e-05,
        "precision_at_1000": 2.6449612403098288e-05,
        "precision_at_20": 0.00013197278911564192,
        "query_delta_l2": 0.051305367291156354,
        "query_p1_cosine": 0.9989042323882437,
        "recall_at_10": 0.0013209055365014377,
        "recall_at_100": 0.0009969363483773863,
        "recall_at_1000": 0.00010753099075111461,
        "recall_at_20": -0.0005444480115971428,
        "support_size": 512.0
      },
      "passed_dense_equivalence": true,
      "source": "ridge_1"
    },
    {
      "deltas": {
        "accuracy": 0.0,
        "candidate_upper_bound": 6.068453884200409e-05,
        "dense_overlap_at_10": 0.0036042714760321504,
        "dense_overlap_at_100": 0.004306986236354948,
        "dense_overlap_at_256": 0.0035311288759690695,
        "dense_overlap_at_50": 0.0038478405315615083,
        "hit_rate_at_10": 0.0,
        "hit_rate_at_100": 0.0,
        "hit_rate_at_1000": 0.0,
        "hit_rate_at_20": -0.0006666666666667043,
        "map_at_10": -0.00013326862272422524,
        "map_at_100": -7.670996606035096e-05,
        "map_at_1000": 3.835802554064127e-05,
        "map_at_20": -0.00017539758877394007,
        "mrr_at_10": -0.0005648148148147403,
        "mrr_at_100": -0.0005866896176008884,
        "mrr_at_1000": -0.0005869117018940662,
        "mrr_at_20": -0.0006091538091538284,
        "ndcg_at_10": -1.342803629345024e-05,
        "ndcg_at_100": -6.66436846553653e-05,
        "ndcg_at_1000": 2.5074001921532307e-05,
        "ndcg_at_20": -0.00021675611962501673,
        "precision_at_10": -6.666666666660381e-05,
        "precision_at_100": -2.7210884354089693e-07,
        "precision_at_1000": 4.911627906977606e-05,
        "precision_at_20": -6.666666666671484e-05,
        "query_delta_l2": 0.01990636150889143,
        "query_p1_cosine": 0.9998744294487436,
        "recall_at_10": -0.00012987772461459102,
        "recall_at_100": 0.0003030666078661648,
        "recall_at_1000": 6.068453884200409e-05,
        "recall_at_20": -0.0007370039942856677,
        "support_size": 512.0
      },
      "passed_dense_equivalence": true,
      "source": "ridge_10"
    }
  ],
  "passed_sources": [
    "ridge_0.001",
    "ridge_0.01",
    "ridge_0.1",
    "ridge_1",
    "ridge_10"
  ],
  "status": "oracle_capacity_exists"
}
```
