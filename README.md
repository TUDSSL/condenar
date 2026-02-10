# TURNER Games Console

This is the official public repository for the TURNER battery-free gaming console.

## System Design Rationale
Battery-free computer gaming offers a vision of sustainable interaction in which games run on hardware that does not require a battery, yet this approach introduces uncertainty due to frequent power failures. Rather than viewing these failures as limitations, this work examines how integrating energy harvesting with application design can encourage users to reimagine and work with such failures, thus shaping behaviour and supporting device use. We present TURNER, a state-of-the-art modular battery-free games console powered by a hand crank and solar cells, created as a research probe to study how energy harvesting mediates the relationship between power and interaction. 

In a subsequent mixed-methods study (N = 60), we explored the influence of energy harvesting on gameplay. Findings show significant variations in harvesting strategies, with interviews surfacing strategies for creating applications that respond to and build on the patterns of system power failure, the ergonomics of energy harvesting, and the value of embedding energy generation into play. Our work offers insights for interactive, sustainable battery-free computers.

## Repository Structure
- Data: This folder contains the data and data analysis scripts used in the TURNER user study. The results of study can be viewed in the work "Connecting Power and Play: Investigating Interactive Energy Harvesting in Battery-Free Gaming"
- Hardware: This folder contains the source files for the hardware used in this project. Schematics, PCBs, and 3D models.
- Software: The folder contains the software used in the TURNER console (including console firmware, apps, and logging software)

## TURNER System Description







## TURNER User Study
The user study of TURNER employed a mixed-methods approach, combining console log data, questionnaire responses, and post-study interviews.

### Quantitative Analysis
A range of statistical tests were used to assess the significance of effects in our log and questionnaire data. Parametric tests were applied when assumptions of normality, from Shapiro-Wilk, and homoscedasticity, from Levene's tests, were met. To analyse differences among more than two groups, we used Analysis of Variance (ANOVA) for between-subjects designs and repeated-measures ANOVA for within-subjects designs when parametric assumptions held, and Kruskal–Wallis or Friedman tests when non-parametric, respectively. Significant results were followed by post-hoc pairwise comparisons using Tukey’s HSD (parametric) or Dunn’s test with Bonferroni correction (non-parametric). For comparisons between two groups, we used independent samples t-tests or Mann-Whitney U tests, as appropriate. 

### Qualitative Analysis
We analysed post-study interviews using inductive thematic analysis, following the six-phase guidance proposed by Braun and Clarke [10] . The interview protocol was co-developed by the first and last authors and reviewed by the fourth author to ensure alignment with the study goals. Interviews were conducted by the first and last authors, and transcripts were produced and analysed by the first author. Coding was inductive, with initial descriptive and in vivo codes generated through repeated engagement with participants’ responses.Codes were iteratively refined and grouped into broader patterns through multiple rounds of analysis, with discussions between the first and fourth authors to clarify code meanings and assess coherence and distinction between themes. 

## How to Cite This Work

The results of this project have been published in a peer-reviewed academic publication (from which all technical figures in this file originate). Details of the publication are as follows.

* **Authors and the project team:** [James Scott Broadhead](https://scholar.google.com/citations?user=sMDFHlMAAAAJ), [Jasper de Winkel](https://jasperdewinkel.com), [Alejandro Cabrerizo Martinez de La Puente](https://www.linkedin.com/in/alejandro-cabrerizo-03780b136), [Himanshu Verma](https://vermahimanshu.com), [Przemysław Pawełczak](http://www.pawelczak.net)
* **Publication title:** _Connecting Power and Play: Investigating Interactive Energy Harvesting in Battery-Free Gaming_
* **Pulication venue:** [Proceedings of the 2026 CHI Conference on Human Factors in Computing Systems (CHI '26), April 13-17, 2026, Barcelona, Spain](https://dl.acm.org/)
* **Link to publication:** https://doi.org/10.1145/3772318.3790831 (Open Access)
* **Link to ACM CHI 2026 conference presentation video:** https://www.youtube.com/watch?

To cite this publication please use the following BiBTeX entry.

```
@article{broadhead:chi:2026:turner,
  title = {Connecting Power and Play: Investigating Interactive Energy Harvesting in Battery-Free Gaming},
  author = {James Scott {Broadhead} and Jasper {de Winkel} and Alejandro Cabrerizo Martinez {de La Puente} and Himanshu {Verma} and Przemys{\l}aw {Pawe{\l}czak}},
  journal = {Proc. ACM Conference on Human Factors in Computing Systems (ACM CHI)},
  month =  apr # " 13--17",
  year = {2026},
  pages = {--},
  address = {Barcelona, Spain},
  publisher = {ACM}
}
```

## Related Websites

* **Project source code:** https://github.com/tudssl/engage
* **Project announcement website:** https://www.freethegameboy.info

## Copyright

Copyright (C) 2026 TU Delft Embedded Systems Group/Sustainable Systems Laboratory.

MIT License or otherwise specified. See [license](https://github.com/TUDSSL/CONDENAR/blob/master/LICENSE) file for details.
