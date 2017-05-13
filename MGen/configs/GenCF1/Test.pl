include "include/default.pl"
include "include/hmean-low.pl"
include "include/rules-low.pl"
#include "include/hmean-high.pl"

# Choose one of algorithms:
# rswa - Random Sliding Windows Approximation (random_seed and random_range are activated automatically). Shuffle not needed
# scan - full (or window) scan all possible combinations (can randomize starting vector with random_seed and random_range)

algorithm = rswa
c_len = 9 # Number of measures in each cantus. Usually 9 to 11
interval = 7-12 # Minimum-maximum chromatic interval in cantus (12 = octave)
key = Am # Music key to use for generation
first_note = A5 # Starting note of each cantus
last_note = A5 # Ending note of each cantus

s_len = 7 # Maximum number of measures to full scan. 6-7 is recommended. Lower values can create less melodic results. Higher values are slow to compute
random_seed = 0 # Seed melody with random numbers. This ensures giving different results if generation is very slow. (automatically enabled for RSWA)
random_range = 0 # Limit scanning to one of possible fast-scan ranges (automatically enabled for RSWA)
shuffle = 1 # If you want to shuffle all canti after generation (can shuffle up to 32000 canti)

# Random SWA
fullscan_max = 10 # Maximum steps length to full scan. If melody is longer, use SWA
approximations = 500 # Maximum number of approximations to run if rpenalty decreases
swa_steps = 3 # Size of Sliding Window Approximation algorithm window in steps
correct_range = 7 # Maximum interval allowed between each source and corrected note

# Technical parameters
calculate_correlation = 0 # Enables correlation calculation algorithm. Slows down generation. Outputs to cf1-cor.csv
calculate_blocking = 0 # Enables blocking flags calculation algorithm. Slows down generation.
calculate_stat = 0 # Enables flag statistics calculation algorithm. Slows down generation.
calculate_ssf = 1 # Enables SWA stuck flags statistics calculation algorithm.
best_rejected = 0 # Show best rejected results if rejecting more than X ms. Set to 0 to disable. Slows down generation. Requires calculate_stat