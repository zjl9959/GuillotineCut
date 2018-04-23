<p style="text-align: center; font-size: 32px; font-weight:bold;">
  Guillotine Cut
</p>



[TOC]



# ROADEF Challenge 2018 Qualification

## Known

### Set

| Set | Description                   | Size        | Element         | Remark                                                         |
| ---- | ---------------------- | ----------- | ------------ | ------------------------------------------------------------ |
| $I$  | (**i**tem) item set | $[1, 200]$  | $i, i'$ |               |
| $S$ | (**s**tack) set of ordered list of items | $[1, 200]$  | $s$ |               |
| $F$ | (**f**law) defect set | $[1, 200]$  | $f$ |               |
| $G$ | (**g**lass) ordered list of raw material | $[1, 200]$ | $g$ | bins |
| $L^{1}$ | layer-1 virtual bin | need good estimation | $l, l'$ |  |
| $L^{2}_{l}$ | layer-2 virtual bin | need good estimation | $m, m'$ |  |
| $L^{3}_{lm}$ | layer-3 virtual bin |need good estimation  | $n, n'$ |  |

### Constant

| Constant | Description                    | Type | Range       | Remark |
| -------- | ------------------------------ | ---- | ----------- | ------ |
| $W$     | the width of the raw material  | real | $(0, 10000]$ | 6000 in challenge |
| $H$      | the height of the raw material | real | $(0, 10000]$ | 3210 in challenge |
| $W^{+}_{1}$ | the max width of every non-trivial L1 virtual bin | real | $(0, W]$ | 3500 in challenge |
| $W^{-}_{1}$ | the min width of every non-trivial L1 virtual bin | real | $(0, W]$ | 100 in challenge |
| $H^{-}_{2}$ | the min height of every non-trivial L2 virtual bin | real | $(0, H]$ | 100 in challenge |
| $W^{-}_{3}$ | the min width of every non-trivial empty L3 virtual bin | real | $(0, W]$ | 20 in challenge |
| $H^{-}_{4}$ | the min height of every non-trivial empty L4 virtual bin | real | $(0, H]$ | 20 in challenge |
| $\Omega_{i}, \Omega_{f}$ | the width of the item $i$ or flaw $f$ | real | $(0, W]$    |  |
| $\Eta_{i}, \Eta_{f}$ | the height of the item $i$ or flaw $f$ | real | $(0, H]$    |        |
| $X_{f}$     | horizontal position of the flaw $f$'s left bottom                        | real | $[0, W - \Omega_{f}]$ | in Cartesian coordinate system |
| $Y_{f}$ | vertical position of the flaw $f$'s left bottom | real | $[0, H - \Eta_{f}]$ | in Cartesian coordinate system |
| $L$ | the total number of L3 virtual bins | int | $[0, 10000]$ | $L = |G| \cdot |L^{1}| \cdot |L^{2}_{l}| \cdot |L^{3}_{lm}|$ |


## Decision

| Variable     | Description                                                | Type | Domain     | Remark                                                     |
| -------------- | ------------------------------------------------------------ | ---- | ------------- | ------------------------------------------------------------ |
| $d_{i}$       | item $i$ is rotated 90 degree            | bool  | $\{0, 1\}$ |  |
| $\omega^{1}_{l}$ | width of the L1 virtual bin $l$ | real | $[0, +\infty)$ | |
| $\eta^{2}_{lm}$ | height of the L2 virtual bin $m$ in L1 virtual bin $l$ | real | $[0, +\infty)$ | |
| $\omega^{3}_{lmn}$ | width of the L3 virtual bin $n$ in L2 virtual bin $(l, m)$ | real | $[0, +\infty)$ |  |
| $\eta^{4+}_{lmn}$ | height of the upper waste in L3 virtual bin $(l, m, n)$ | real | $[0, +\infty)$ |  |
| $\eta^{4-}_{lmn}$ | height of the lower waste in L3 virtual bin $(l, m, n)$ | real | $[0, +\infty)$ |  |
| $\gamma$ | width of the residual in the last used bin | real | $[0, +\infty)$ | |
| $\tau^{2}_{lm}$ | L2 virtual bin $(l, m)$ is non-trivial by positive height | bool | $\{0, 1\} $ | bool variable to implement semi variable |
| $\tau^{3}_{lmn}$ | L3 virtual bin $(l, m, n)$ is non-trivial by positive width | bool | $\{0, 1\} $ | bool variable to implement semi variable |
| $\tau^{4+}_{lmn}$ | upper waste in L3 virtual bin $(l, m, n)$ is non-trivial | bool | $\{0, 1\} $ | bool variable to implement semi variable |
| $\tau^{4-}_{lmn}$ | lower waste in L3 virtual bin $(l, m, n)$ is non-trivial | bool | $\{0, 1\} $ | bool variable to implement semi variable |
| $e$ | there is residual in raw material | bool | $\{0, 1\}$ | bool variable to implement semi variable |
| $p$ | there are items placed in raw material | bool | $\{0, 1\} $ |  |
| $p_{l}$ | there are items placed in L1 virtual bin $l$ | bool | $\{0, 1\}$ | |
| $p_{lm}$ | there are items placed in L2 virtual bin $(l, m)$ | bool | $\{0, 1\}$ | |
| $p_{lmn}$ | there are items placed in L3 virtual bin $(l, m, n)$ | bool | $\{0, 1\}$ | |
| $p_{lmni}$ | item $i$ is placed in L3 virtual bin $(l, m, n)$ | bool | $\{0, 1\}$ | |
| $c_{lmnf}$ | L3 virtual bin $(l, m, n)$ contains flaw $f$ | bool | $\{0, 1\}$ | |
| $c^{k}_{lmnf}$ | L3 virtual bin $(l, m, n)$ is not on the $k^\textrm{th}$ side of flaw $f$ | bool | $\{0, 1\}$ | $k \in \{1\textrm{:right}, 2\textrm{:left}, 3\textrm{:up}, 4\textrm{:down}\}$ |
| $o_{i}$ | the sequence number of item $i$ to be produced | real | $[0, +\infty)$ |  |

notes on solution representation.
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
this leads to more bins to be considered in the sequencing constraints.

(2) make single L4 virtual bin which can place item and two L4 virtual bins above/below it which can be waste only, and at least one of the waste bins should be trivial.
this leads to more bins to be considered in the width/height bounding constraints.

### Convention and Function

- since the constraints are independent on each raw material, the dimension $g$ in some constants and variables is omitted.
  - flaw position $X_{gf}, Y_{gf}, \Omega_{gf}, \Eta_{gf}$
  - virtual bins $L^{1}_{g}, L^{2}_{gl}, L^{3}_{glm}$
  - virtual bin size $\omega^{1}_{gl}, \eta^{2}_{glm}, \omega^{3}_{glmn}$
  - trivial or empty bin $\tau^{2}_{glm}, \tau^{3}_{glmn}, e_{g}$
  - item inclusion $p_{g}, p_{gl}, p_{glm}, p_{glmn}, p_{glmni}$
  - flaw inclusion $c_{glmnf}, c^{k}_{glmnf}$
- a trivial bin means a bin with 0 width or height, which is nothing.
- an empty bin means a bin without item placed in, which could be waste or residual.
- assume $\Omega_{i} \ge \Eta_{i}$ is true on all items (this can be done easily by **preprocessing**), but may be false on flaws.
- ~~assume all items' width and height are no less than $W^{-}_{3}, H^{-}_{4}$.~~ ==(then one may simply make $\omega^{3}_{lmn} \in \{0\} \cup [W^{-}_{3}, W]$)==
- ~~assume all items' width and height are no less than $W^{-}_{1}, H^{-}_{2}, W^{-}_{3}, H^{-}_{4}$.~~ ==(check which constraints will be trivial when it is true)==
- define function $\textrm{w}(i) = \Omega_{i} \cdot (1 - d_{i}) + \Eta_{i} \cdot d_{i}$ to indicate the actual width of item $i$ regarding its rotation.
- define function $\textrm{h}(i) = \Eta_{i} \cdot (1 - d_{i}) + \Omega_{i} \cdot d_{i}$ to indicate the actual height of item $i$ regarding its rotation.
- define function $\textrm{x}(l, m, n) = \sum\limits_{l' \in L^{1}, l' < l} \omega^{1}_{l'} + \sum\limits_{n' \in L^{3}_{lm}, n' < n} \omega^{3}_{lmn'}$ to indicate the horizontal position of L3 virtual bin $(l, m, n)$'s left bottom.
- define function $\textrm{y}(l, m, n) = \sum\limits_{m' \in L^{2}_{l}, m' < m} \eta^{2}_{lm'} + \eta^{4-}_{lmn}$ to indicate the vertical position of L3 virtual bin $(l, m, n)$'s left bottom.
- define function $\textrm{seq}(g, l, m, n) = |L^{3}_{lm}| \cdot (|L^{2}_{l}| \cdot (|L^{1}| \cdot g + l) + m) + n$ to indicate the produced order of L3 virtual bin $(l, m, n)$ in bin $g$.
- define function $\textrm{next}(i)$ to indicate the next item of $i$ in its stack.
- define function $\textrm{next}(g)$ to indicate the next bin of $g$ in the queue $G$.


## Objective

### minimize the wasted raw material OWG (wasted glass)

reduce the geometrical loss of the cutting patterns, i.e., reduce the used glass, as the area of items are fixed.
only the glass at right of the last 1-cut performed in the last cutting pattern can be reused (regard as residual instead of waste).

$$
\min W \cdot \sum_{g \in G} p_{g} - (W - \gamma)
$$

### maximize the area of placed items OPI (placed items)

$$
\max \sum_{i \in I} \sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} w_{i} \cdot h_{i} \cdot p_{lmni}
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

- **HIP1.A (L1 item placement)** if there are items placed in L1 virtual bin $l$.
$$
|I| \cdot p_{l} \ge \sum_{i \in I} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} p_{lmni}, \quad \forall l \in L^{1}
$$
- **HIP2.A (L2 item placement)** if there are items placed in L2 virtual bin $(l, m)$.
$$
|I| \cdot p_{lm} \ge \sum_{i \in I} \sum_{n \in L^{3}_{lm}} p_{lmni}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}
$$
- **HIP3.A (L3 item placement)** if there are items placed in L3 virtual bin $(l, m, n)$.
$$
|I| \cdot p_{lmn} \ge \sum_{i \in I} p_{lmni}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$

- **HRP (residual position)** locating the residual on the last bin.
$$
\sum_{l \in L^{1}_{g}} \omega_{gl} - W \cdot (p_{g} - p_{g'}) \le \gamma \le \sum_{l \in L^{1}_{g}} \omega_{gl} + W \cdot (p_{g} - p_{g'}), \quad \forall g, g' \in G, g' = \textrm{next}(g)
$$

- **HTW1 (L1 total width)** the sum of all L1 virtual bin's width should be equal to the width of the raw material or leave enough width for the waste.
==only the rightmost waste on the last bin is residual? then the $W^{+}_{1}$ limit should be considered in the rest area!==
$$
W \cdot (1 - e) \le \sum_{l \in L^{1}} \omega^{1}_{l} \le W - W^{-}_{3} \cdot e
$$
- **HTH2 (L2 total height)** the sum of all L2 virtual bin's height should be equal to the height of the raw material.
$$
\sum_{m \in L^{2}_{l}} \eta^{2}_{lm} = H, \quad \forall l \in L^{1}
$$
- **HTW3 (L3 total width)** the sum of all L3 virtual bin's width should not exceed the width of its covering L1 virtual bin.
$$
\sum_{n \in L^{3}_{l}} \omega^{3}_{lmn} = \omega^{1}_{l}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}
$$


- **HHF (horizontal fitting)** the width of the item $i$ should be equal to the width of a L3 virtual bin $(l, m, n)$ if $i$ is placed in.
$W​$ on the left side can be reduced to $\Omega_{i}​$ to tighten the bound, but it may not accelerate the optimization.
$W$ on the right side can be any value that is no less than $\omega^{3}_{lmn}$.
$$
\textrm{w}(i) - W \cdot (1 - p_{lmni}) \le \omega^{3}_{lmn} \le \textrm{w}(i) + W \cdot (1 - p_{lmni}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$
- **HVF (vertical fitting)** the height of the item $i$ should be equal to the height of a L3 virtual bin $(l, m, n)$ if $i$ is placed in.
$H$ on the left side can be reduced to $\Omega_{i}$ to tighten the bound, but it may not accelerate the optimization.
$H$ on the right side can be any value that is no less than $\eta^{2}_{lm}$.
$$
\textrm{h}(i) - H \cdot (1 - p_{lmni}) \le \eta^{2}_{lm} - \eta^{4+}_{lmn} - \eta^{4-}_{lmn} \le \textrm{h}(i) + H \cdot (1 - p_{lmni}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$

- **HWB1 (L1 width bound)** L1 virtual bin $l$'s width should not exceed the width limits if there are items placed in.
the left side can be replaced by a series of constraints applied on each L2 virtual bin, L3 virtual bin or item in L3 virtual bin.
$$
W^{-}_{1} \cdot p_{l} \le \omega^{1}_{l} \le W^{+}_{1}, \quad \forall l \in L^{1}
$$
- **HMH2 (L2 min height)** L2 virtual bin $(l, m)$'s height should not be less than the min height.
$H$ on the left side can be reduced to $H^{-}_{4}$ to tighten the bound.
$$
H^{-}_{2} \cdot p_{lm} \le \eta^{2}_{lm}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}
$$
$$
H^{-}_{4} \cdot \tau^{2}_{lm} - H \cdot p_{lm} \le \eta^{2}_{lm} \le H \cdot \tau^{2}_{lm} + H \cdot p_{lm}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}
$$
- **HMW3 (L3 min width)** L3 virtual bin $(l, m, n)$'s width should not be less than the min width if it is empty.
the left side can be replaced by a series of constraints applied on each item in L3 virtual bin.
if all items' width and height are no less than $W^{-}_{3}, H^{-}_{4}$, then $W \cdot p_{lmn}$ can be omitted.
$$
W^{-}_{3} \cdot \tau^{3}_{lmn} - W \cdot p_{lmn} \le \omega^{3}_{lmn} \le W \cdot \tau^{3}_{lmn} + W \cdot p_{lmn}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$
- **HMH4 (L4 min height)** the height of wastes in L3 virtual bin $(l, m, n)$ should not be less than the min height.
$$
H^{-}_{4} \cdot \tau^{4+}_{lmn} \le \eta^{4+}_{lmn} \le H \cdot \tau^{4+}_{lmn}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$
$$
H^{-}_{4} \cdot \tau^{4-}_{lmn} \le \eta^{4-}_{lmn} \le H \cdot \tau^{4-}_{lmn}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$

- **HMW (max waste)** there should be no more than 1 waste in L3 virtual bin $(l, m, n)$.
$$
\tau^{4+}_{lmn} + \tau^{4-}_{lmn} \le 1, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$

- **HSP (single placement)** each item should be placed in exactly 1 L3 virtual bin.
change $=$ to $\le$ if **OPI (placed items)** is enabled.
$$
\sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} p_{lmni} = 1, \quad \forall i \in I
$$
- **HEP (exclusive placement)** there should be no more than 1 item placed in single L3 virtual bin $(l, m, n)$.
it will be a trivial constraint if **HDF (defect free)** is enabled.
$$
\sum_{i \in I} p_{lmni} \le 1, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$

- **HDF (defect free)** there should be no item placed in L3 virtual bin $(l, m, n)$ if it contains defects.
enabling it will make **HEP (exclusive placement)** a trivial constraint.
$$
\sum_{i \in I} p_{lmni} \le 1 - c_{lmnf}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
- **HDC (defect containing)** L3 virtual bin $(l, m, n)$ contains flaw $f$ if it is not on any side of flaw $f$.
$$
c_{lmnf} \ge \bigwedge^{4}_{k = 1} c^{k}_{lmnf}
\ \Leftrightarrow\ 
3 + c_{lmnf} \ge \sum^{4}_{k = 1} c^{k}_{lmnf}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
- **HDD (defect direction)** L3 virtual bin $(l, m, n)​$ is on some sides of flaw $f​$.
$$
\textrm{x}(l, m, n) + W \cdot c^{1}_{lmnf} \ge X_{f} + \Omega_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
$$
\textrm{x}(l, m, n) + \omega^{3}_{lmn} + W \cdot c^{2}_{lmnf} \le X_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
$$
\textrm{y}(l, m, n) + H \cdot c^{3}_{lmnf} \ge Y_{f} + \Eta_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
$$
\textrm{y}(l, m, n) + \omega^{2}_{lm} + H \cdot c^{4}_{lmnf} \le Y_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$

- **HIO (item order)** if item $i$ is placed in L3 virtual bin $(l, m, n)$ then its next item in stack should only be in L3 virtual bin $(l', m', n')$ where $\textrm{seq}(l, m, n) < \textrm{seq}(l', m', n')$.
it will make **HIL (item label)**, **HLO (label order)** trivial constraints.
$$
\begin{split}
1 - p_{lmni} \ge p_{l'm'n'i'}, \quad &\forall l, l' \in L^{1}, \forall m, m' \in L^{2}_{l}, \forall n, n' \in L^{3}_{lm}, \forall i, i' \in I, i' = \textrm{next}(i),\\
&(l' \le l - 1) \vee ((l' = l) \wedge ((m' \le m - 1) \vee ((m = m') \wedge (n' \le n - 1))
\end{split}
$$
- **HIL (item label)** if item $i$ is placed in L3 virtual bin $(l, m, n)$ then its ordinal number is $\textrm{seq}(l, m, n)$.
if **HLO (label order)** is also enabled, they will make **HIO (item order)** a trivial constraint.
$$
\textrm{seq}(l, m, n) - L \cdot (1 - p_{lmni}) \le o_{i} \le \textrm{seq}(l, m, n) + L \cdot (1 - p_{lmni}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$
- **HLO (label order)** the value of items' labels in the same stack should be in increasing order.
if **HIL (item label)** is also enabled, they will make **HIO (item order)** a trivial constraint.
$$
o_{i} + 1 \le o_{i'}, \quad \forall i, i' \in I, i' = \textrm{next}(i)
$$

- **HGO (glass order)** the bins should used one by one without skipping some bins.
$$
p_{g} \ge p_{g'}, \quad \forall g, g' \in G, g' = \textrm{next}(g)
$$

- **HHO1.O (L1 horizontal order)** put all trivial bins to the rightmost.
set $W = 1$ to make the wider virtual bins comes first if there is no defect.
$$
W \cdot \omega^{1}_{l} \ge \omega^{1}_{l'}, \quad \forall l, l' \in L^{1}, l' = l + 1
$$
- **HVO2.O (L2 vertical order)** put all trivial bins to the upmost.
set $H = 1$ to make the higher virtual bins comes first if there is no defect.
$$
H \cdot \omega^{2}_{lm} \ge \omega^{2}_{l'm'}, \quad \forall l \in L^{1}, \forall m, m' \in L^{2}_{l}, m' = m + 1
$$
- **HHO3.O (L3 horizontal order)** put all trivial bins to the rightmost.
set $W = 1$ to make the wider virtual bins comes first if there is no defect.
$$
W \cdot \omega^{3}_{lmn} \ge \omega^{3}_{l'm'n'}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n, n' \in L^{3}_{lm}, n' = n + 1
$$
