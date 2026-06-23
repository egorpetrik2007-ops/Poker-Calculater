// ============================================================
//  No-Limit Texas Hold'em Engine
//  Optimised for Monte-Carlo simulations:
//    - no heap allocations in the hot path (fixed arrays)
//    - no exceptions
//    - no static locals / globals  →  fully thread-safe
//    - Game::reset() reuses the same object between hands
// ============================================================

#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <cstring>

// ─── Enums ──────────────────────────────────────────────────

enum class Suit { clubs, diamonds, hearts, spades, None };

enum class Stage { beginning, preflop, flop, turn, river, showdown };

enum class Combinations {
    OlderCard, Pair, TwoPairs, ThreeofKind,
    Straight, Flush, FullHouse, FourofKind,
    StraightFlush, RoyalFlush
};

// ─── Card ───────────────────────────────────────────────────
//  rank 0          → empty / no card
//  rank 2..14      → Two … Ace
//  No exceptions: Setnumber clamps silently to valid range.

class Card {
private:
    Suit suit_;
    int  rank_;          // 0 = empty, 2-14 = card

public:
    // Default: empty card
    Card() : suit_(Suit::None), rank_(0) {}

    Card(Suit s, int r) : suit_(Suit::None), rank_(0) {
        Setsuit(s);
        Setnumber(r);
    }

    Card(const Card& o) = default;
    Card& operator=(const Card& o) = default;

    bool operator<(const Card& o) const { return rank_ < o.rank_; }

    Suit Getsuit()  const { return suit_; }
    int  Getnumber() const { return rank_; }

    void Setsuit(Suit s) { suit_ = s; }

    // 0 → empty; 2-14 → valid rank; anything else → clamped to 0
    void Setnumber(int r) {
        if (r == 0 || (r >= 2 && r <= 14)) rank_ = r;
        else                                rank_ = 0;
    }

    bool isEmpty() const { return rank_ == 0; }

    void printcard() const {
        if (isEmpty()) { std::cout << "[  ]"; return; }
        switch (rank_) {
        case 11: std::cout << "J"; break;
        case 12: std::cout << "Q"; break;
        case 13: std::cout << "K"; break;
        case 14: std::cout << "A"; break;
        default: std::cout << rank_; break;
        }
        switch (suit_) {
        case Suit::clubs:    std::cout << "♣ "; break;
        case Suit::diamonds: std::cout << "♦ "; break;
        case Suit::hearts:   std::cout << "♥ "; break;
        case Suit::spades:   std::cout << "♠ "; break;
        default:             std::cout << "? "; break;
        }
    }
};

// ─── Deck ───────────────────────────────────────────────────
//  52 cards on the stack; no heap allocation.

class Deck {
private:
    Card        cards_[52];
    int         top_;          // index of next card to deal (counts down)
    std::mt19937 rng_;

    void fill() {
        const Suit suits[4] = {
            Suit::clubs, Suit::diamonds, Suit::hearts, Suit::spades
        };
        int idx = 0;
        for (Suit s : suits)
            for (int r = 2; r <= 14; r++)
                cards_[idx++] = Card(s, r);
        top_ = 52;
    }

public:
    Deck() : top_(0), rng_(std::random_device{}()) { fill(); }

    // Reset + shuffle — call before each hand in Monte-Carlo
    void reset() {
        fill();
        shuffle();
    }

    void shuffle() {
        std::shuffle(cards_, cards_ + 52, rng_);
        top_ = 52;
    }

    // Returns empty card if deck is exhausted (no exception)
    Card dealCard() {
        if (top_ <= 0) return Card{};   // empty sentinel
        return cards_[--top_];
    }

    int remaining() const { return top_; }

    void printdeck() const {
        std::cout << "\nКолода (" << top_ << " карт):\n";
        for (int i = 0; i < top_; i++) { cards_[i].printcard(); }
        std::cout << '\n';
    }
};

// ─── Player ─────────────────────────────────────────────────

class Player {
private:
    Card hand_[2];
    int  stack_;         // chip stack
    int  betInRound_;    // chips committed in current betting round
    bool isFolded_;
    char name_[32];      // fixed buffer — no std::string heap alloc

public:
    Player() { reset(); name_[0] = '\0'; }

    // ── identity ──
    void setName(const char* n) {
        int i = 0;
        while (n[i] && i < 31) { name_[i] = n[i]; i++; }
        name_[i] = '\0';
    }
    const char* getName() const { return name_; }

    // ── cards ──
    void setHand(const Card& c1, const Card& c2) {
        hand_[0] = c1;
        hand_[1] = c2;
    }
    const Card& getCard(int i) const { return hand_[i]; }

    // ── budget ──
    void  addStack(int chips) { stack_ += chips; }
    int   getStack()  const { return stack_; }
    int   getBetInRound() const { return betInRound_; }

    // Returns actual chips moved (may be less if all-in)
    int bet(int amount) {
        int actual = (amount > stack_) ? stack_ : amount;
        stack_ -= actual;
        betInRound_ += actual;
        return actual;
    }

    void clearBetInRound() { betInRound_ = 0; }

    // ── actions ──
    void fold() { isFolded_ = true; }
    bool folded() const { return isFolded_; }
    bool active() const { return !isFolded_ && stack_ >= 0; }

    // ── per-hand reset (keeps name & stack) ──
    void resetHand() {
        hand_[0] = Card{};
        hand_[1] = Card{};
        isFolded_ = false;
        betInRound_ = 0;
    }

    // ── full reset (new player at table) ──
    void reset() {
        hand_[0] = Card{};
        hand_[1] = Card{};
        stack_ = 0;
        betInRound_ = 0;
        isFolded_ = false;
    }

    void printhand() const {
        std::cout << name_ << ": ";
        hand_[0].printcard();
        hand_[1].printcard();
        std::cout << "  stack=" << stack_;
        if (isFolded_) std::cout << " [FOLD]";
        std::cout << '\n';
    }
};

// ─── Table ──────────────────────────────────────────────────

class Table {
private:
    Player seats_[10];
    Card board_[5];
    int boardCount_;
    int numPlayers_;
    int pot_;

public:
    Table() { clearTable(); }

    // ── seating ──
    bool sitPlayer(const Player& p) {
        if (numPlayers_ >= 10) return false;
        seats_[numPlayers_++] = p;
        return true;
    }

    Player& getPlayer(int i) { return seats_[i]; }
    const Player& getPlayer(int i) const { return seats_[i]; }
    int numPlayers() const { return numPlayers_; }

    // ── board ──
    bool addBoardCard(const Card& c) {
        if (boardCount_ >= 5) return false;
        board_[boardCount_++] = c;
        return true;
    }
    const Card& getBoardCard(int i) const { return board_[i]; }
    int boardCount() const { return boardCount_; }

    // ── pot ──
    int  getPot()  const { return pot_; }
    void addToPot(int chips) { pot_ += chips; }
    void clearPot() { pot_ = 0; }

    // ── full clear (between hands, keeps seated players) ──
    void clearTable() {
        boardCount_ = 0;
        pot_ = 0;
        for (int i = 0; i < 5; i++) board_[i] = Card{};
        // reset each player's hand (keep stack & name)
        for (int i = 0; i < numPlayers_; i++) seats_[i].resetHand();
    }

    // ── full reset (remove players too) ──
    void fullReset() {
        numPlayers_ = 0;
        clearTable();
    }

    void printTable() const {
        std::cout << "\n=== TABLE (pot=" << pot_ << ") ===\n";
        std::cout << "Board: ";
        for (int i = 0; i < boardCount_; i++) board_[i].printcard();
        std::cout << '\n';
        for (int i = 0; i < numPlayers_; i++) seats_[i].printhand();
    }
};

// ─── CombinationCalculator ──────────────────────────────────
//  Evaluates best 5-card hand from 2+5 cards.
//  No heap allocations: tieBreakers is a fixed int[5].

class CombinationCalculator {
private:
    bool cards_[4][15];       // [suit][rank]
    int  dignity_[15];        // count of each rank across all suits
    int  countOfSuit_[4];

    Combinations combination_;
    int  tieBreakers_[5];
    int  tbCount_;

    int count4_, count3_, count2_;
    bool isFlush_;
    int  flushSuit_;
    int  straightHigh_;
    int  straightFlushHigh_;

    static int suitIndex(Suit s) {
        switch (s) {
        case Suit::clubs:    return 0;
        case Suit::diamonds: return 1;
        case Suit::hearts:   return 2;
        case Suit::spades:   return 3;
        default:             return 0;
        }
    }

    void pushTB(int v) {
        if (tbCount_ < 5) tieBreakers_[tbCount_++] = v;
    }

    // ── analysis ──
    void analyzeRawData() {
        for (int i = 0; i < 4; i++) {
            if (countOfSuit_[i] >= 5) { isFlush_ = true; flushSuit_ = i; break; }
        }
        for (int i = 14; i >= 2; i--) {
            if (dignity_[i] == 4) count4_++;
            else if (dignity_[i] == 3) count3_++;
            else if (dignity_[i] == 2) count2_++;
        }

        int streak = 0, sfStreak = 0;
        // Treat A as low (rank=1) too for wheel straight
        for (int i = 14; i >= 1; i--) {
            int r = (i == 1) ? 14 : i;
            if (dignity_[r] > 0) {
                streak++;
                if (streak >= 5 && straightHigh_ == -1)
                    straightHigh_ = (i == 1) ? 5 : (i + 4);
            }
            else {
                streak = 0;
            }
            if (isFlush_ && cards_[flushSuit_][r]) {
                sfStreak++;
                if (sfStreak >= 5 && straightFlushHigh_ == -1)
                    straightFlushHigh_ = (i == 1) ? 5 : (i + 4);
            }
            else {
                sfStreak = 0;
            }
        }
    }

    bool tryRoyalFlush() {
        if (straightFlushHigh_ == 14) {
            combination_ = Combinations::RoyalFlush;
            pushTB(14);
            return true;
        }
        return false;
    }
    bool tryStraightFlush() {
        if (straightFlushHigh_ != -1) {
            combination_ = Combinations::StraightFlush;
            pushTB(straightFlushHigh_);
            return true;
        }
        return false;
    }
    bool tryFourOfAKind() {
        if (count4_ > 0) {
            combination_ = Combinations::FourofKind;
            int quads = -1, kicker = -1;
            for (int i = 14; i >= 2; i--) {
                if (dignity_[i] == 4 && quads == -1) quads = i;
                else if (dignity_[i] > 0 && kicker == -1) kicker = i;
            }
            pushTB(quads); pushTB(kicker);
            return true;
        }
        return false;
    }
    bool tryFullHouse() {
        if ((count3_ > 0 && count2_ > 0) || count3_ > 1) {
            combination_ = Combinations::FullHouse;
            int trips = -1, pair = -1;
            for (int i = 14; i >= 2; i--)
                if (dignity_[i] == 3 && trips == -1) { trips = i; break; }
            for (int i = 14; i >= 2; i--)
                if (i != trips && dignity_[i] >= 2 && pair == -1) { pair = i; break; }
            pushTB(trips); pushTB(pair);
            return true;
        }
        return false;
    }
    bool tryFlush() {
        if (isFlush_) {
            combination_ = Combinations::Flush;
            for (int i = 14; i >= 2 && tbCount_ < 5; i--)
                if (cards_[flushSuit_][i]) pushTB(i);
            return true;
        }
        return false;
    }
    bool tryStraight() {
        if (straightHigh_ != -1) {
            combination_ = Combinations::Straight;
            pushTB(straightHigh_);
            return true;
        }
        return false;
    }
    bool tryThreeOfAKind() {
        if (count3_ > 0) {
            combination_ = Combinations::ThreeofKind;
            int trips = -1;
            for (int i = 14; i >= 2; i--)
                if (dignity_[i] == 3 && trips == -1) { trips = i; break; }
            pushTB(trips);
            for (int i = 14; i >= 2 && tbCount_ < 3; i--)
                if (dignity_[i] > 0 && i != trips) pushTB(i);
            return true;
        }
        return false;
    }
    bool tryTwoPairs() {
        if (count2_ >= 2) {
            combination_ = Combinations::TwoPairs;
            int p1 = -1, p2 = -1;
            for (int i = 14; i >= 2; i--) {
                if (dignity_[i] == 2) {
                    if (p1 == -1) p1 = i;
                    else if (p2 == -1) { p2 = i; break; }
                }
            }
            pushTB(p1); pushTB(p2);
            for (int i = 14; i >= 2 && tbCount_ < 3; i--)
                if (dignity_[i] > 0 && i != p1 && i != p2) pushTB(i);
            return true;
        }
        return false;
    }
    bool tryPair() {
        if (count2_ > 0) {
            combination_ = Combinations::Pair;
            int pair = -1;
            for (int i = 14; i >= 2; i--)
                if (dignity_[i] == 2 && pair == -1) { pair = i; break; }
            pushTB(pair);
            for (int i = 14; i >= 2 && tbCount_ < 4; i--)
                if (dignity_[i] > 0 && i != pair) pushTB(i);
            return true;
        }
        return false;
    }
    void setOlderCard() {
        combination_ = Combinations::OlderCard;
        for (int i = 14; i >= 2 && tbCount_ < 5; i--)
            if (dignity_[i] > 0) pushTB(i);
    }

    void calculateCards() {
        analyzeRawData();
        if (tryRoyalFlush())    return;
        if (tryStraightFlush()) return;
        if (tryFourOfAKind())   return;
        if (tryFullHouse())     return;
        if (tryFlush())         return;
        if (tryStraight())      return;
        if (tryThreeOfAKind())  return;
        if (tryTwoPairs())      return;
        if (tryPair())          return;
        setOlderCard();
    }

public:
    CombinationCalculator() { reset(); }

    void reset() {
        std::memset(cards_, 0, sizeof(cards_));
        std::memset(dignity_, 0, sizeof(dignity_));
        std::memset(countOfSuit_, 0, sizeof(countOfSuit_));
        std::memset(tieBreakers_, 0, sizeof(tieBreakers_));
        tbCount_ = 0;
        combination_ = Combinations::OlderCard;
        count4_ = count3_ = count2_ = 0;
        isFlush_ = false;
        flushSuit_ = -1;
        straightHigh_ = -1;
        straightFlushHigh_ = -1;
    }

    void addCard(const Card& c) {
        if (c.isEmpty()) return;
        int s = suitIndex(c.Getsuit());
        int r = c.Getnumber();
        if (r < 2 || r > 14) return;
        cards_[s][r] = true;
        dignity_[r]++;
        countOfSuit_[s]++;
    }

    // playerHand[2]  +  board[boardCount] cards
    void evaluate(const Card(&hand)[2],
        const Card* board, int boardCount)
    {
        reset();
        addCard(hand[0]);
        addCard(hand[1]);
        for (int i = 0; i < boardCount; i++) addCard(board[i]);
        calculateCards();
    }

    // ── comparisons ──
    bool operator==(const CombinationCalculator& o) const {
        if (combination_ != o.combination_) return false;
        if (tbCount_ != o.tbCount_)     return false;
        for (int i = 0; i < tbCount_; i++)
            if (tieBreakers_[i] != o.tieBreakers_[i]) return false;
        return true;
    }
    bool operator!=(const CombinationCalculator& o) const { return !(*this == o); }
    bool operator>(const CombinationCalculator& o) const {
        if (combination_ != o.combination_)
            return static_cast<int>(combination_) > static_cast<int>(o.combination_);
        int n = (tbCount_ < o.tbCount_) ? tbCount_ : o.tbCount_;
        for (int i = 0; i < n; i++) {
            if (tieBreakers_[i] != o.tieBreakers_[i])
                return tieBreakers_[i] > o.tieBreakers_[i];
        }
        return tbCount_ > o.tbCount_;
    }
    bool operator<(const CombinationCalculator& o) const { return o > *this; }

    Combinations getCombination() const { return combination_; }
};

// ─── Game ───────────────────────────────────────────────────
//  Manages one table session.
//  reset() reuses the object for the next hand (Monte-Carlo friendly).

class Game {
private:
    Table  table_;
    Stage  stage_;
    Deck   deck_;
    int    dealerPos_;    // button position

    // ── small/big blind parameters ──
    int smallBlind_;
    int bigBlind_;

    // ── per-hand state ──
    int  currentBet_;     // highest bet this betting round
    int  actionPos_;      // index of player whose turn it is

    // ── helpers ──
    int  nextActive(int from) const {
        int n = table_.numPlayers();
        for (int i = 1; i <= n; i++) {
            int idx = (from + i) % n;
            if (!table_.getPlayer(idx).folded()) return idx;
        }
        return from;
    }

    int activeCount() const {
        int c = 0;
        for (int i = 0; i < table_.numPlayers(); i++)
            if (!table_.getPlayer(i).folded()) c++;
        return c;
    }

    void postBlinds() {
        int n = table_.numPlayers();
        if (n < 2) return;

        int sbPos = (dealerPos_ + 1) % n;
        int bbPos = (dealerPos_ + 2) % n;

        int sb = table_.getPlayer(sbPos).bet(smallBlind_);
        table_.addToPot(sb);

        int bb = table_.getPlayer(bbPos).bet(bigBlind_);
        table_.addToPot(bb);

        currentBet_ = bigBlind_;
        actionPos_ = (bbPos + 1) % n;   // UTG acts first preflop
    }

    void dealHoleCards() {
        int n = table_.numPlayers();
        for (int i = 0; i < n; i++) {
            Card c1 = deck_.dealCard();
            Card c2 = deck_.dealCard();
            table_.getPlayer(i).setHand(c1, c2);
        }
    }

    void dealBoardCards(int count) {
        for (int i = 0; i < count; i++)
            table_.addBoardCard(deck_.dealCard());
    }

public:
    Game(int smallBlind = 1, int bigBlind = 2)
        : stage_(Stage::beginning),
        dealerPos_(0),
        smallBlind_(smallBlind),
        bigBlind_(bigBlind),
        currentBet_(0),
        actionPos_(0)
    {
    }

    // ── seat a player (only before game starts) ──
    bool addPlayer(Player& p) {
        if (stage_ != Stage::beginning) return false;
        return table_.sitPlayer(p);
    }

    // ── start a new hand ──
    void startHand() {
        if (table_.numPlayers() < 2) return;

        deck_.reset();         // reshuffle
        table_.clearTable();   // clear board & hands, keep stacks
        stage_ = Stage::preflop;
        currentBet_ = 0;

        postBlinds();
        dealHoleCards();
    }

    // ── advance to the next stage ──
    void nextStage() {
        switch (stage_) {
        case Stage::preflop:
            stage_ = Stage::flop;
            dealBoardCards(3);
            currentBet_ = 0;
            resetBetsInRound();
            actionPos_ = nextActive(dealerPos_);
            break;

        case Stage::flop:
            stage_ = Stage::turn;
            dealBoardCards(1);
            currentBet_ = 0;
            resetBetsInRound();
            actionPos_ = nextActive(dealerPos_);
            break;

        case Stage::turn:
            stage_ = Stage::river;
            dealBoardCards(1);
            currentBet_ = 0;
            resetBetsInRound();
            actionPos_ = nextActive(dealerPos_);
            break;

        case Stage::river:
            stage_ = Stage::showdown;
            doShowdown();
            break;

        default: break;
        }
    }

    void resetBetsInRound() {
        for (int i = 0; i < table_.numPlayers(); i++)
            table_.getPlayer(i).clearBetInRound();
    }

    // ── betting actions ──
    void actionFold() {
        table_.getPlayer(actionPos_).fold();
        if (activeCount() == 1) {
            // everyone else folded — award pot to last player
            for (int i = 0; i < table_.numPlayers(); i++) {
                if (!table_.getPlayer(i).folded()) {
                    table_.getPlayer(i).addStack(table_.getPot());
                    table_.clearPot();
                    stage_ = Stage::beginning;
                    return;
                }
            }
        }
        advanceAction();
    }

    void actionCall() {
        Player& p = table_.getPlayer(actionPos_);
        int need = currentBet_ - p.getBetInRound();
        int actual = p.bet(need);
        table_.addToPot(actual);
        advanceAction();
    }

    void actionCheck() {
        // only valid when currentBet_ == player's betInRound
        advanceAction();
    }

    // amount = TOTAL bet this round (not additional)
    void actionRaise(int totalBet) {
        Player& p = table_.getPlayer(actionPos_);
        int need = totalBet - p.getBetInRound();
        int actual = p.bet(need);
        table_.addToPot(actual);if (p.getBetInRound() > currentBet_) {
            currentBet_ = p.getBetInRound();
        }
        advanceAction();
    }

    void advanceAction() {
        actionPos_ = nextActive(actionPos_);
    }

    // ── showdown ──
    void doShowdown() {
        int n = table_.numPlayers();
        if (n == 0) return;

        // Evaluate each active player
        CombinationCalculator calcs[10];
        for (int i = 0; i < n; i++) {
            if (table_.getPlayer(i).folded()) continue;

            // Build hand array for the player
            const Card hand[2] = {
                table_.getPlayer(i).getCard(0),
                table_.getPlayer(i).getCard(1)
            };
            // board as raw pointer + count
            // We use a local fixed array to comply with evaluate's API
            Card board[5];
            int  bc = table_.boardCount();
            for (int j = 0; j < bc; j++) board[j] = table_.getBoardCard(j);

            calcs[i].evaluate(hand, board, bc);
        }

        // Find the best hand
        int bestIdx = -1;
        for (int i = 0; i < n; i++) {
            if (table_.getPlayer(i).folded()) continue;
            if (bestIdx == -1 || calcs[i] > calcs[bestIdx]) bestIdx = i;
        }

        // Collect winners (split pot if tied)
        int  winners[10];
        int  winCount = 0;
        for (int i = 0; i < n; i++) {
            if (table_.getPlayer(i).folded()) continue;
            if (calcs[i] == calcs[bestIdx]) winners[winCount++] = i;
        }

        // Distribute pot
        int pot = table_.getPot();
        int share = pot / winCount;
        int remainder = pot % winCount;

        for (int w = 0; w < winCount; w++) {
            int extra = (w == 0) ? remainder : 0;   // give remainder to first winner
            table_.getPlayer(winners[w]).addStack(share + extra);
        }
        table_.clearPot();

        // Print result
        std::cout << "\n=== SHOWDOWN ===\n";
        for (int w = 0; w < winCount; w++) {
            std::cout << "Победитель: "
                << table_.getPlayer(winners[w]).getName()
                << " (+" << share + (w == 0 ? remainder : 0) << ")\n";
        }
        if (winCount > 1) std::cout << "Split pot!\n";

        stage_ = Stage::beginning;
        dealerPos_ = (dealerPos_ + 1) % n;   // move button
    }

    // ── full reset for Monte-Carlo (reuse object, keep seats) ──
    void reset() {
        deck_.reset();
        table_.clearTable();
        stage_ = Stage::beginning;
        currentBet_ = 0;
        actionPos_ = 0;
    }

    // ── accessors ──
    Stage  getStage()  const { return stage_; }
    Table& getTable() { return table_; }
    const Table& getTable() const { return table_; }

    void printState() const { table_.printTable(); }
};

// ─── main ────────────────────────────────────────────────────

int main() {
    // Example: 3 players, 100-chip stacks, blinds 1/2
    Game game(1, 2);

    Player p1, p2, p3;
    p1.setName("Alice"); p1.addStack(100);
    p2.setName("Bob");   p2.addStack(100);
    p3.setName("Carol"); p3.addStack(100);

    game.addPlayer(p1);
    game.addPlayer(p2);
    game.addPlayer(p3);
    game.printState();
    // ── Hand 1 ──
    game.startHand();
    game.printState();

    // Simple scripted actions (preflop): everyone calls
    game.actionCall();   // UTG calls
    game.actionCall();   // SB calls
    game.actionCheck();  // BB checks

    game.nextStage();    // → Flop
    game.printState();

    game.actionCheck();
    game.actionCheck();
    game.actionCheck();

    game.nextStage();    // → Turn
    game.nextStage();    // → River
    game.nextStage();    // → Showdown (evaluates + pays out)

    // ── Reset for next hand (Monte-Carlo style) ──
    game.reset();
    game.startHand();
    game.printState();

    return 0;
}