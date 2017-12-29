typedef struct tradeParams{
    int num_shares, num_days;
    double alpha, Pi_1, Pi_2, p1, p2, rho;
    double *optimal;
    std::vector<int>solvec;
}tradeParams;

void *execute(void *);