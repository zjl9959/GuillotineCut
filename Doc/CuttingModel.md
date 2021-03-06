<p style="text-align: center; font-size: 32px; font-weight:bold;">
  Guillotine Cut
</p>



[TOC]



# ROADEF Challenge 2018 Qualification

## Known

### Set

| Set | Description                   | Size        | Element         | Remark                                                         |
| ---- | ---------------------- | ----------- | ------------ | ------------------------------------------------------------ |
| $I$ | (**i**tem) item set | $[1, 700]$ | $i, i'$ |               |
| $S$ | (**s**tack) set of ordered list of items | $[1, |I|]$ | $s$ |               |
| $F$ | (**f**law) defect set | $[0, 8 \cdot |G|]$ | $f$ |               |
| $G$ | (**g**lass) ordered list of plate | $[1, 100]$ | $g, g'$ | bins |
| $L^{1}$ | layer-1 virtual bin | need good estimation | $l, l'$ | **parameter** |
| $L^{2}_{l}$ | layer-2 virtual bin | need good estimation | $m, m'$ | **parameter** |
| $L^{3}_{lm}$ | layer-3 virtual bin |need good estimation  | $n, n'$ | **parameter** |
| $L^{4}_{lmn}$ | layer-4 virtual bin |2  | $k, k'$ |  |

### Constant

| Constant | Description                    | Type | Range       | Remark |
| -------- | ------------------------------ | ---- | ----------- | ------ |
| $W$     | the width of the plate  | real | $(0, 10000]$ | 6000 in challenge |
| $H$      | the height of the plate | real | $(0, 10000]$ | 3210 in challenge |
| $W^{+}_{1}$ | the max width of every non-trivial L1 virtual bin | real | $(0, W]$ | 3500 in challenge |
| $W^{-}_{1}$ | the min width of every non-trivial L1 virtual bin | real | $(0, W]$ | 100 in challenge |
| $H^{-}_{2}$ | the min height of every non-trivial L2 virtual bin | real | $(0, H]$ | 100 in challenge |
| $W^{-}_{3}$ | the min width of every non-trivial empty L3 virtual bin | real | $(0, W]$ | 20 in challenge |
| $H^{-}_{4}$ | the min height of every non-trivial empty L4 virtual bin | real | $(0, H]$ | 20 in challenge |
| $\Omega_{i}, \Omega_{f}$ | the width of the item $i$ or flaw $f$ | real | $[0, W]$  | $\Omega_{i} \ne 0$ |
| $\Eta_{i}, \Eta_{f}$ | the height of the item $i$ or flaw $f$ | real | $[0, H]$ | $\Eta_{i} \ne 0$ |
| $X_{f}$     | horizontal position of the flaw $f$'s left bottom                        | real | $[0, W - \Omega_{f}]$ | in Cartesian coordinate system |
| $Y_{f}$ | vertical position of the flaw $f$'s left bottom | real | $[0, H - \Eta_{f}]$ | in Cartesian coordinate system |
| $L$ | the total number of L3 virtual bins | int | $[0, 10000]$ | $L = |G| \cdot |L^{1}| \cdot |L^{2}_{l}| \cdot |L^{3}_{lm}|$ |
| $U$ | the estimated optimal utilization ratio | real | $[0, 1]$ | **parameter** |


## Decision

| Variable     | Description                                                | Type | Domain     | Remark                                                     |
| -------------- | ------------------------------------------------------------ | ---- | ------------- | ------------------------------------------------------------ |
| $d_{i}$       | item $i$ is rotated 90 degree            | bool  | $\{0, 1\}$ |  |
| $p$ | there are items placed in plate | bool | $\{0, 1\} $ |  |
| $p_{l}$ | there are items placed in L1 virtual bin $l$ | bool | $\{0, 1\}$ | |
| $p_{lm}$ | there are items placed in L2 virtual bin $(l, m)$ | bool | $\{0, 1\}$ | |
| $p_{lmn}$ | there are items placed in L3 virtual bin $(l, m, n)$ | bool | $\{0, 1\}$ | |
| $p_{lmnk}$ | there are items placed in L4 virtual bin $(l, m, n, k)$ | bool | $\{0, 1\}$ | |
| $p_{lmnki}$ | item $i$ is placed in L4 virtual bin $(l, m, n, k)$ | bool | $\{0, 1\}$ | |
| $\omega^{1}_{l}$ | width of the L1 virtual bin $l$ | real | $[0, +\infty)$ | |
| $\eta^{2}_{lm}$ | height of the L2 virtual bin $m$ in L1 virtual bin $l$ | real | $[0, +\infty)$ | |
| $\omega^{3}_{lmn}$ | width of the L3 virtual bin $n$ in L2 virtual bin $(l, m)$ | real | $[0, +\infty)$ |  |
| $\eta^{4}_{lmnk}$ | height of the L4 virtual bin $k$ in L3 virtual bin $(l, m, n)$ | real | $[0, +\infty)$ |  |
| $\tau^{3}_{lmn}$ | L3 virtual bin $(l, m, n)$ is non-trivial by positive width | bool | $\{0, 1\} $ | bool variable to implement semi variable |
| $\tau^{4}_{lmnk}$ | L4 virtual bin $(l, m, n, k)$ is non-trivial by positive height | bool | $\{0, 1\} $ | bool variable to implement semi variable |
| $\gamma$ | width of the used area in the last used bin | real | $[0, +\infty)$ | |
| $e$ | there is residual in plate | bool | $\{0, 1\}$ | bool variable to implement semi variable |
| $c_{lmnkf}$ | L4 virtual bin $(l, m, n, k)$ covers flaw $f$ | bool | $\{0, 1\}$ | |
| $c^{1}_{f}$ | the residual in plate covers flaw $f$ | bool | $\{0, 1\}$ |  |
| $o_{i}$ | the sequence number of item $i$ to be produced | real | $[0, +\infty)$ |  |

### Convention and Function

- since the constraints are independent on each plate, the dimension $g$ in some constants and variables is omitted.
  - flaw position $X_{gf}, Y_{gf}, \Omega_{gf}, \Eta_{gf}$
  - virtual bins $L^{1}_{g}, L^{2}_{gl}, L^{3}_{glm}$
  - virtual bin size $\omega^{1}_{gl}, \eta^{2}_{glm}, \omega^{3}_{glmn}$
  - trivial or empty bin $\tau^{3}_{glmn}, \tau^{4}_{glmnk}, e_{g}$
  - item inclusion $p_{g}, p_{gl}, p_{glm}, p_{glmn}, p_{glmnk}, p_{glmnki}$
  - flaw inclusion $c_{glmnf}, c^{1}_{gf}$
- an empty bin means a bin without item placed in, which could be waste or residual.
- a trivial bin means a bin with 0 width or height, which is nothing, and it will always be empty.
- assume $\Omega_{i} \ge \Eta_{i}$ is true on all items (this can be done easily by **preprocessing**), but may be false on flaws.
- ~~assume all items' width and height are no less than $W^{-}_{3}, H^{-}_{4}$.~~ ==(then one may simply make $\omega^{3}_{lmn} \in \{0\} \cup [W^{-}_{3}, W]$)==
- ~~assume all items' width and height are no less than $W^{-}_{1}, H^{-}_{2}, W^{-}_{3}, H^{-}_{4}$.~~ ==(check which constraints will be trivial when it is true)==
- define function $\textrm{w}(i) = \Omega_{i} \cdot (1 - d_{i}) + \Eta_{i} \cdot d_{i}$ to indicate the actual width of item $i$ regarding its rotation.
- define function $\textrm{h}(i) = \Eta_{i} \cdot (1 - d_{i}) + \Omega_{i} \cdot d_{i}$ to indicate the actual height of item $i$ regarding its rotation.
- define function $\textrm{x}(l, m, n) = \sum\limits_{l' \in L^{1}, l' < l} \omega^{1}_{l'} + \sum\limits_{n' \in L^{3}_{lm}, n' < n} \omega^{3}_{lmn'}$ to indicate the horizontal position of L3 virtual bin $(l, m, n)$'s left bottom.
- define function $\textrm{y}(l, m, n, k) = \sum\limits_{m' \in L^{2}_{l}, m' < m} \eta^{2}_{lm'} + \sum\limits_{k' \in L^{4}_{lmn}, k' < k} \eta^{4}_{lmnk'}$ to indicate the vertical position of L4 virtual bin $(l, m, n, k)$'s left bottom.
- define function $\textrm{seq}(g, l, m, n, k) = (((g \cdot |L^{1}| + l) \cdot |L^{2}_{l}| + m) \cdot |L^{3}_{lm}| + n) \cdot |L^{4}_{lmn}| + k$ to indicate the produced order of L4 virtual bin $(l, m, n, k)$ in bin $g$.
- define function $\textrm{next}(i)$ to indicate the next item of $i$ in its stack.
- define function $\textrm{next}(g)$ to indicate the next bin of $g$ in the queue $G$.
- define function $\textrm{p}(i) = \sum\limits_{l \in L^{1}} \sum\limits_{m \in L^{2}_{l}} \sum\limits_{n \in L^{3}_{lm}} \sum\limits_{n \in L^{4}_{lmn}} p_{lmnki}$ to indicate whether item $i$ is placed in any plate.


## Objective

### minimize the width of used glass **OGW (glass width)**

the original objective of the problem.
only the glass at right of the last 1-cut performed in the last cutting pattern can be reused (regard as residual instead of waste).

$$
\min W \cdot \sum_{g \in G} p_{g} - (W - \gamma)
$$

### maximize the area of placed items **OPI (placed items)**

the optima is an feasible solution to the original problem.

$$
\max \sum_{i \in I} \Omega_{i} \cdot \Eta_{i} \cdot \textrm{p}(i)
$$

### minimize the wasted glass **OWG (wasted glass)**

it is able to be used in **iterated models** that determine the placement of items in a single L1 virtual bin at each iteration.
if every item must be placed (**HSP (single placement)** takes the $=$ form), it is the same as **OGW (glass width)**.

$$
\min W \cdot \sum_{g \in G} p_{g} - (W - \gamma) - \sum_{i \in I} \Omega_{i} \cdot \Eta_{i} \cdot \textrm{p}(i)
$$

### maximize the utilization ratio **OUR (utilization ratio)**

it is able to be used in **iterated models** that determine the placement of items in a single L1 virtual bin at each iteration.
if every item must be placed (**HSP (single placement)** takes the $=$ form), it is the same as **OGW (glass width)**.

$$
\max \frac{\sum_{i \in I} \Omega_{i} \cdot \Eta_{i} \cdot \textrm{p}(i)}{W \cdot \sum_{g \in G} p_{g} - (W - \gamma)}
$$

it can be transformed to linear form by introducing the estimation of the optimal utilization ratio $U$.
an additional constraint to force putting at least 1 item may be needed if $U$ is overestimated.
substituting the $U$ with the objective value and re-optimizing may get better results until $U$ converges.

$$
\max \sum_{i \in I} \Omega_{i} \cdot \Eta_{i} \cdot \textrm{p}(i) - U \cdot W \cdot \sum_{g \in G} p_{g} - (W - \gamma)
$$

it can also be regarded as minimizing the estimated width of used glass **OEW (estimated width)**.

$$
\min W \cdot \sum_{g \in G} p_{g} - (W - \gamma) + U^{-1} \cdot \sum_{i \in I} \Omega_{i} \cdot \Eta_{i} \cdot (1 - \textrm{p}(i))
$$


## Constraint

there are several notations about the constraints.

- hard constraints
  - prefixed with **H** in the tag
  - constraints in the original problem that must be satisfied
- soft constraints
  - prefixed with **S** in the tag
  - objectives in the original problem
  - cut-off scores to the low-priority objectives in case the high-priority objectives are over dominant
- auxiliary constraints
  - prefixed with **A** in the tag
  - additional constraints for converting non-linear constraints into linear ones
- optional constraints
  - prefixed with **.O** in the tag
  - user cuts or constraints that are very likely to be ignored when changing requirement or implementation
- lazy constraints
  - suffixed with **.L** in the tag
  - constraints which are recommended to be added lazily

all of the following constraints must be satisfied.

- **HIP0.A (L0 item placement)** if there are items placed in the plate.
  [?]*the left side can be omitted if **OPI (placed items)** is disabled.*
  $$
  p \le \sum_{l \in L^{1}} p_{l} \le |I| \cdot p
  $$

- **HIP1.A (L1 item placement)** if there are items placed in L1 virtual bin $l$.
  [?]*the left side can be omitted if **OPI (placed items)** is disabled.*
  $$
  p_{l} \le \sum_{m \in L^{2}_{l}} p_{lm} \le |I| \cdot p_{l}, \quad \forall l \in L^{1}
  $$

- **HIP2.A (L2 item placement)** if there are items placed in L2 virtual bin $(l, m)$.
  [?]*the left side can be omitted if **OPI (placed items)** is disabled.*
  $$
  p_{lm} \le \sum_{n \in L^{3}_{lm}} p_{lmn} \le |I| \cdot p_{lm}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}
  $$

- **HIP3.A (L3 item placement)** if there are items placed in L3 virtual bin $(l, m, n)$.
  [?]*the left side can be omitted if **OPI (placed items)** is disabled.*
  $$
  p_{lmn} \le \sum_{k \in L^{4}_{lmn}} p_{lmnk} \le |I| \cdot p_{lmn}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
  $$

- **HIP4.A (L4 item placement)** if there are items placed in L4 virtual bin $(l, m, n, k)$.
  [?]*the left side can be omitted if **OPI (placed items)** is disabled.*
  $|I|$ on the right side can be 1 if **HEP (exclusive placement)** is enabled.
  $$
  p_{lmnk} \le \sum_{i \in I} p_{lmnki} \le |I| \cdot p_{lmnk}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k \in L^{4}_{lmn}
  $$

- **HRP (residual position)** locating the residual on the last bin and get the width of the used area.
  if $g$ is the last plate, then replace $p_{g'}$ with 0 since the past-the-last plate can never be used.
  $$
  \sum_{l \in L^{1}_{g}} \omega_{gl} - W \cdot (1 - p_{g} + p_{g'}) \le \gamma \le \sum_{l \in L^{1}_{g}} \omega_{gl} + W \cdot (1 - p_{g} + p_{g'}), \quad \forall g, g' \in G, g' = \textrm{next}(g)
  $$

- **HTW1 (L1 total width)** the sum of all L1 virtual bin's width should be equal to the width of the plate or leave enough width for the waste.
  ==only the rightmost waste on the last bin is residual? then the $W^{+}_{1}$ limit should be considered in the rest area!==
  $$
  W \cdot (1 - e) \le \sum_{l \in L^{1}} \omega^{1}_{l} \le W - W^{-}_{3} \cdot e
  $$

- **HTH2 (L2 total height)** the sum of all L2 virtual bin's height should be equal to the height of the plate.
  $$
  \sum_{m \in L^{2}_{l}} \eta^{2}_{lm} = H, \quad \forall l \in L^{1}
  $$

- **HTW3 (L3 total width)** the sum of all L3 virtual bin's width should not exceed the width of its covering L1 virtual bin.
  $$
  \sum_{n \in L^{3}_{l}} \omega^{3}_{lmn} = \omega^{1}_{l}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}
  $$

- **HTH4 (L4 total height)** the sum of all L4 virtual bin's height should be equal to the height of its covering L2 virtual bin.
  $$
  \sum_{k \in L^{4}_{lmn}} \eta^{4}_{lmnk} = \eta^{2}_{lm}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{2}_{lm}
  $$

- **HHF (horizontal fitting)** the width of the item $i$ should be equal to the width of a L4 virtual bin $(l, m, n, k)$ if $i$ is placed in.
  $W​$ on the left side can be reduced to $\Omega_{i}​$ to tighten the bound, but it may not accelerate the optimization.
  $W$ on the right side can be any value that is no less than $\omega^{3}_{lmn}$.
  $$
  \textrm{w}(i) - W \cdot (1 - p_{lmnki}) \le \omega^{3}_{lmn} \le \textrm{w}(i) + W \cdot (1 - p_{lmnki}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k \in L^{4}_{lmn}
  $$

- **HVF (vertical fitting)** the height of the item $i$ should be equal to the height of a L4 virtual bin $(l, m, n, k)$ if $i$ is placed in.
  $H$ on the left side can be reduced to $\Omega_{i}$ to tighten the bound, but it may not accelerate the optimization.
  $H$ on the right side can be any value that is no less than $\eta^{4}_{lmnk}$.
  $$
  \textrm{h}(i) - H \cdot (1 - p_{lmnki}) \le \eta^{4}_{lmnk} \le \textrm{h}(i) + H \cdot (1 - p_{lmnki}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k \in L^{4}_{lmn}
  $$

- **HWB1 (L1 width bound)** L1 virtual bin $l$'s width should not exceed the width limits if there are items placed in.
  the left side can be replaced by a series of constraints applied on each L2 virtual bin, L3 virtual bin or item in L3 virtual bin.
  $$
  W^{-}_{1} \cdot p_{l} \le \omega^{1}_{l} \le W^{+}_{1}, \quad \forall l \in L^{1}
  $$

- **HMH2 (L2 min height)** L2 virtual bin $(l, m)$'s height should not be less than the min height if there are items placed in.
  $$
  H^{-}_{2} \cdot p_{lm} \le \eta^{2}_{lm}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}
  $$

- **HMW3 (L3 min width)** L3 virtual bin $(l, m, n)$'s width should not be less than the min width if it is empty.
  if all items' width are no less than $W^{-}_{3}$, then $W \cdot p_{lmnk}$ can be omitted.
  $W$ on the left side can be reduced to $W^{-}_{3}$ to tighten the bound.
  $$
  W^{-}_{3} \cdot \tau^{3}_{lmn} - W \cdot p_{lmn} \le \omega^{3}_{lmn} \le W \cdot \tau^{3}_{lmn} + W \cdot p_{lmn}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
  $$

- **HMH4 (L4 min height)** L4 virtual bin $(l, m, n, k)$'s height should not be less than the min height if it is empty.
  if all items' height are no less than $H^{-}_{4}$, then $H \cdot p_{lmnk}$ can be omitted.
  $H$ on the left side can be reduced to $H^{-}_{4}$ to tighten the bound.
  $$
  H^{-}_{4} \cdot \tau^{4}_{lmnk} - H \cdot p_{lmnk} \le \eta^{4}_{lmnk} \le H \cdot \tau^{4}_{lmnk} + H \cdot p_{lmnk}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k \in L^{4}_{lmn}
  $$

- **HTI.O (triviality inheritance)** L4 virtual bin $(l, m, n, k)$ should be trivial if its parent L3 virtual bin $(l, m, n)$ is trivial.
  $$
  \tau^{4}_{lmnk} \le \tau^{3}_{lmn}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k \in L^{4}_{lmn}
  $$

- **HSP (single placement)** each item should be placed in exactly 1 L4 virtual bin.
  the $=$ should be $\le$ if **OPI (placed items)** is enabled.
  $$
  \sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} \sum_{n \in L^{4}_{lmn}} p_{lmnki} = 1, \quad \forall i \in I
  $$

- **HEP (exclusive placement)** there should be no more than 1 item placed in single L4 virtual bin $(l, m, n, k)$.
  it will be a trivial constraint if **HDF (defect free)** is enabled.
  $$
  \sum_{i \in I} p_{lmnki} \le 1, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall n \in L^{4}_{lmn}
  $$

- **HDF (defect free)** there should be no item placed in L4 virtual bin $(l, m, n, k)$ if it contains defects.
  enabling it will make **HEP (exclusive placement)** a trivial constraint.
  $$
  \sum_{i \in I} p_{lmnki} \le 1 - c_{lmnkf}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall n \in L^{4}_{lmn}, \forall f \in F
  $$

- **HDB (defect bypassing)** cutting through defects is forbidden, i.e., each defect should be covered by single L4 virtual bin or the residual entirely.
  $$
  c^{1}_{f} + \sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} \sum_{n \in L^{4}_{lmn}} c_{lmnkf} = 1, \quad \forall f \in F
  $$

- **HDC (defect covering)** $c_{lmnkf}$ should be false if L4 virtual bin $(l, m, n, k)$ does not cover flaw $f$ (this constraint can not guarantee its converse-negative proposition).
  - for the L4 virtual bin.
    $$
    \textrm{x}(l, m, n) - W \cdot (1 - c_{lmnkf}) \le X_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall n \in L^{4}_{lmn}, \forall f \in F
    $$
    $$
    \textrm{x}(l, m, n) + \omega^{3}_{lmn} + W \cdot (1 - c_{lmnkf}) \ge X_{f} + \Omega_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall n \in L^{4}_{lmn}, \forall f \in F
    $$
    $$
    \textrm{y}(l, m, n, k) - H \cdot (1 - c_{lmnkf}) \le Y_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall n \in L^{4}_{lmn}, \forall f \in F
    $$
    $$
    \textrm{y}(l, m, n, k) + \eta^{4}_{lmnk} + H \cdot (1 - c_{lmnkf}) \ge Y_{f} + \Eta_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall n \in L^{4}_{lmn}, \forall f \in F
    $$

  - for the residual.
    $$
    \sum_{l \in L^{1}} \omega^{1}_{l} - W \cdot (1 - c^{1}_{f}) \le X_{f}, \quad \forall f \in F
    $$

- **HDT.O (defect fitting)** defects can be covered by non-trivial bins only.
  $$
  c_{lmnkf} \le \tau^{4}_{lmnk}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall n \in L^{4}_{lmn}, \forall f \in F
  $$

- **HIO (item order)** if item $i$ is placed in L4 virtual bin $(l, m, n, k)$ then its next item in stack should only be in L4 virtual bin $(l', m', n', k')$ where $\textrm{seq}(l, m, n, k) < \textrm{seq}(l', m', n', k')$.
  it will make **HIL (item label)** & **HLO (label order)** trivial constraints.
  the right side can be $\sum p_{l'm'n'k'i'}$ where $(l', m', n', k') \in \{ (l', m', n', k') | \textrm{seq}(l', m', n', k') < \textrm{seq}(l, m, n, k) \}$ to reduce the number of constraints.
  $$
  \begin{equation}
  \begin{split}
  1 - p_{lmnki} \ge p_{l'm'n'k'i'}, \quad &\forall l, l' \in L^{1}, \forall m, m' \in L^{2}_{l}, \forall n, n' \in L^{3}_{lm}, \forall i, i' \in I, i' = \textrm{next}(i),\\
  &(l' \le l - 1) \vee ((l' = l) \wedge ((m' \le m - 1) \vee ((m = m') \wedge ((n' \le n - 1) \vee ((n = n') \wedge (k \le k'))))))
  \end{split}
  \end{equation}
  $$

- **HIL (item label)** if item $i$ is placed in L4 virtual bin $(l, m, n, k)$ then its ordinal number is $\textrm{seq}(l, m, n, k)$.
  if **HLO (label order)** is also enabled, they will make **HIO (item order)** a trivial constraint.
  $$
  \textrm{seq}(l, m, n, k) - L \cdot (1 - p_{lmnki}) \le o_{i} \le \textrm{seq}(l, m, n, k) + L \cdot (1 - p_{lmnki}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k \in L^{4}_{lmn}
  $$

- **HLO (label order)** the value of items' labels in the same stack should be in increasing order.
  if **HIL (item label)** is also enabled, they will make **HIO (item order)** a trivial constraint.
  $$
  o_{i} + 1 \le o_{i'}, \quad \forall i, i' \in I, i' = \textrm{next}(i)
  $$

- **HPO.O (placement order)** an item can not be placed if its preceding item is not placed.
  it can be omitted if **HSP (single placement)** takes the $=$ sign, i.e., every item must be placed.
  $$
  \sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} \sum_{n \in L^{4}_{lmn}} p_{lmnki} \ge \sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} \sum_{n \in L^{4}_{lmn}} p_{lmnki'}, \quad \forall i, i' \in I, i' = \textrm{next}(i)
  $$

- **HGO.O (glass order)** the bins should used one by one without skipping some bins.
  $$
  p_{g} \ge p_{g'}, \quad \forall g, g' \in G, g' = \textrm{next}(g)
  $$

- **HHO1.O (L1 horizontal order)** put all trivial bins to the rightmost.
  $$
  \tau^{3}_{lmn} \ge \tau^{3}_{l'mn}, \quad \forall l, l' \in L^{1}, l' = l + 1, m = 0, n = 0
  $$
  the following one may result in numeric issue which is deprecated.
  set $W = 1$ to make the wider virtual bins comes first if there is no defect and order.
  $$
  W \cdot \omega^{1}_{l} \ge \omega^{1}_{l'}, \quad \forall l, l' \in L^{1}, l' = l + 1
  $$

- **HVO2.O (L2 vertical order)** put all trivial bins to the upmost.
  $$
  \tau^{4}_{lmnk} \ge \tau^{4}_{lm'nk}, \quad \forall l \in L^{1}, \forall m, m' \in L^{2}_{l}, m' = m + 1, n = 0, k = 0
  $$
  the following one may result in numeric issue which is deprecated.
  set $H = 1$ to make the higher virtual bins comes first if there is no defect and order.
  $$
  H \cdot \eta^{2}_{lm} \ge \eta^{2}_{lm'}, \quad \forall l \in L^{1}, \forall m, m' \in L^{2}_{l}, m' = m + 1
  $$

- **HHO3.O (L3 horizontal order)** put all trivial bins to the rightmost.
  $$
  \tau^{3}_{lmn} \ge \tau^{3}_{lmn'}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n, n' \in L^{3}_{lm}, n' = n + 1
  $$
  the following one may result in numeric issue which is deprecated.
  set $W = 1$ to make the wider virtual bins comes first if there is no defect and order.
  $$
  W \cdot \omega^{3}_{lmn} \ge \omega^{3}_{lmn'}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n, n' \in L^{3}_{lm}, n' = n + 1
  $$

- **HVO4.O (L4 vertical order)** put all trivial bins to the upmost.
  $$
  \tau^{4}_{lmnk} \ge \tau^{4}_{lmnk'}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k, k' \in L^{4}_{lmn}, k' = k + 1
  $$
  the following one may result in numeric issue which is deprecated.
  set $H = 1$ to make the higher virtual bins comes first if there is no defect and order.
  $$
  H \cdot \eta^{4}_{lmnk} \ge \eta^{4}_{lmnk'}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k, k' \in L^{4}_{lmn}, k' = k + 1
  $$

- **HBM1.O (L1 bin merging)** there should not be contiguous non-trivial empty L1 virtual bins.
  ==it may cut off the optima if some defects make an area wider than $W^{+}_{1}$ that can not place any item.==
  it may cut more symmetric solutions if **HHO1.O (L1 horizontal order)** is enabled.
  $$
  p_{l} + p_{l'} \ge \tau^{3}_{l'mn}, \quad \forall l, l' \in L^{1}, l' = l + 1, m = 0, n = 0
  $$

- **HBM2.O (L2 bin merging)** there should not be contiguous non-trivial empty L2 virtual bins.
  it may cut more symmetric solutions if **HVO2.O (L2 vertical order)** is enabled.
  $$
  p_{lm} + p_{lm'} \ge \tau^{4}_{lm'nk}, \quad \forall l \in L^{1}, \forall m, m' \in L^{2}_{l}, m' = m + 1, n = 0, k = 0
  $$

- **HBM3.O (L3 bin merging)** there should not be contiguous non-trivial empty L3 virtual bins.
  it may cut more symmetric solutions if **HHO3.O (L3 horizontal order)** is enabled.
  $$
  p_{lmn} + p_{lmn'} \ge \tau^{3}_{lmn'}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n, n' \in L^{3}_{lm}, n' = n + 1
  $$

- **HBM4.O (L4 bin merging)** there should not be contiguous non-trivial empty L4 virtual bins.
  it may cut more symmetric solutions if **HVO4.O (L4 vertical order)** is enabled.
  $$
  p_{lmnk} + p_{lmnk'} \ge \tau^{4}_{lmnk'}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall k, k' \in L^{4}_{lmn}, k' = k + 1
  $$

- **HAB0.O (L0 area bound)** covered area should be less than the area of the plate.
  $$
  \sum_{i \in I} \Omega_{i} \cdot \Eta_{i} \cdot \sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} \sum_{n \in L^{4}_{lmn}} p_{glmnki} \le W \cdot H, \quad \forall g \in G
  $$

- **HAB1.O (L1 area bound)** covered area should be less than the area of the L1 virtual bin.
  $$
  \sum_{i \in I} \Omega_{i} \cdot \Eta_{i} \cdot \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} \sum_{n \in L^{4}_{lmn}} p_{lmnki} \le W^{+}_{1} \cdot H, \quad \forall l \in L^{1}
  $$

- **HMW.O (min width)** the total width of used area multiplies the plate height should be not less than the total covered area.
  the left side can be replaced with $H \cdot \sum\limits_{l \in L^{1}} \omega^{1}_{l}$ to tighten the bound, but it may not improve the performance.
  $$
  H \cdot (W \cdot \sum_{g \in G} p_{g} - (W - \gamma)) \ge \sum_{i \in I} \Omega_{i} \cdot \Eta_{i} \cdot \textrm{p}(i)
  $$



## Notes

- solution representation.
```
        +---+   +---+          trivial waste-->+===+   +---+
        |///|   |   |<--waste                  |///|   |   |
 item-->|///|   |   |                   item-->|///|   |   |<--waste
        +---+   +---+                          +---+   +---+
waste-->|   |   |///|                  waste-->|   |   |///|<--item
        |   |   |///|<--item                   |   |   |///|
        +---+   +---+                          +---+   +===+<--trivial waste
             (1)                                    (2)
```
(1) make two L4 virtual bins and both of them can place an item.
this leads to more bins to consider in the sequencing constraints.
this leads to less bins to consider in width/height bounding constraints and defect bypassing constraints.

(2) make single L4 virtual bin which can place item and two L4 virtual bins above/below it which can be waste only, and at least one of the waste bins should be trivial.
this leads to more bins to be consider in the width/height bounding constraints and defect bypassing constraints.
this leads to less bins to consider in sequencing constraints.
==this may cuts off the optima if there are some items with $(\Omega_{i} = \Omega_{i'}) \vee (\Omega_{i} = \Eta_{i'}) \vee (\Eta_{i} = \Omega_{i'}) \vee (\Eta_{i} = \Eta_{i'})$.==

- **(not) on some side** vs. **cover** vs. **overlap/contain**.
```
        +-----------+
        |        +--|--+
        +---+    |  3  |
    +---+ 1 |    +--|--+
    | 0 +---+ +---+ | +---+
    +---+     | 2 | | | 4 |
        |     +---+ | +---+
        +-----------+
```
on the left side of the bin: 0.
not on the left side of the bin: 1, 2, 3, 4.
on the right side of the bin : 4.
not on the right side of the bin: 0, 1, 2, 3.
covered by the bin: 1, 2.
overlapped with the bin: 1, 2, 3.

- cut through defects is forbidden even if they are surrounded by wastes.
```
                1-cut                      3-cut                      3-cut           
                 |                          |                          |              
                 V                          V                          V              
          +---+--+---+                  +---+---+                  +---+---+          
          |///|  |   |                  |///|///|                  |   |   |          
          |///|  D   |          4-cut ->+---+///|                  |   D   |          
          |///|  |   |                  |   |///|                  |   |   |          
          |///|  +---+<- 2-cut          +-D-+---+<- 2-cut          |   +---+<- 4-cut  
  2-cut ->+---+--+///|                  |   |///|                  |   |///|          
          |//////|///|          4-cut ->+---+///|          4-cut ->+---+///|          
          |//////|///|                  |///|///|                  |///|///|          
          +------+---+                  +---+---+                  +---+---+          
```
