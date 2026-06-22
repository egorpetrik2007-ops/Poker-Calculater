	#include <iostream>
	#include <string>
	#include <vector>
	#include <algorithm> 
	#include <random>
 
	using namespace std;

	enum class Suit{ clubs , diamonds, hearts, spades};
	enum Stage { preflop, flop, tern , river, showdown};
	enum class Combinations {
	OlderCard,
	Pair,
	TwoPairs,
	ThreeofKind,	
	Straight,
	Flush,
	FullHouse,
	FourofKind,
	Straight_Flash,
	Royal_Flash
	};

	class Card {
	private:
		Suit suit;
		int number;
	public:
		Card() {
			suit = Suit::clubs;
			number = 2;
		}
		Card(const Card& other) {
			suit = other.suit;
			number = other.number;
		}
		Card& operator= (const Card& other) {
			if (this != &other) { 
				suit = other.suit;
				number = other.number; 
			}
			return *this;
		}
		bool operator<(const Card& other) const {
			return this->number < other.number;
		}

		Card(Suit suit, int number) {
			Setsuit(suit);
			Setnumber(number);
		}

		Suit Getsuit() const{ return suit; }
		int Getnumber() const { return number; }
		void Setsuit(Suit suit) { this->suit = suit; }
		void Setnumber(int number) { 
			if (number > 1 && number < 15) this->number = number;
			else throw invalid_argument("Некорректное достоинство карты!");
		}
		void printcard() const{
			switch (number) {
			case 11:cout << "J ";break;
			case 12:cout << "Q ";break;
			case 13:cout << "K ";break;
			case 14:cout << "A ";break;
			default: cout << number << " ";break;	
			}
			switch (suit) {
			case Suit::clubs: cout << "Крести" << "\t";break;
			case Suit::diamonds : cout << "Бубе" << "\t";break;
			case Suit::hearts: cout << "Чирва" << "\t";break;
			case Suit::spades: cout << "Пики" << "\t";break;
			}

		}
	};
	class Deck {
	private:
		vector <Card> cards;
	public:
		Deck() {
			Suit allsuits[] = { Suit::clubs, Suit::diamonds,Suit::hearts,Suit::spades };
			for (Suit s : allsuits)
				for (int n = 2; n <= 14; n++)
					cards.push_back(Card(s, n));

		}
		void shuffleDeck() {
			random_device rd;
			mt19937 gen(rd());
			shuffle(cards.begin(), cards.end(), gen);
		}
		void printdeck() const {
			cout << endl << "Ваша колода:" << endl;
			for (const auto& card : cards)
				card.printcard();
		}
		Card dealCard() {
			if (cards.empty()) {
				throw out_of_range("Колода пуста!");
			}
			Card temp = cards.back();
			cards.pop_back();
			return temp;
		}
	};
	class Player {
	private:
		Card Hand[2];
	
	public:
		Player(Card &cart1, Card &cart2) {
			Hand[0] = cart1;
			Hand[1] = cart2;
		}

		void setfc(Suit suit , int numer) {
			Hand[1].Setsuit(suit);
			Hand[1].Setnumber(numer);
		}
		void setnc(const Card& cart) { Hand[0] = cart; }
		void setnc(Suit suit, int numer) {
			Hand[0].Setsuit(suit);
			Hand[0].Setnumber(numer);
		}
		void setfc(const Card& cart) { Hand[1] = cart; }

		void printhand() const {
			cout << "У вас на руках:" << endl << "1 карта ";
			Hand[0].printcard();
			cout << "2 карта ";
			Hand[1].printcard();
		}

		void getfromdeck(Deck& deck) {
			setnc(deck.dealCard());
			setfc(deck.dealCard());
		}

		const Card& getF() const { return Hand[0]; }
		const Card& getS() const { return Hand[1]; }
	};
	class Table {
	private:
		int pot = 0;
			Stage stage;
			unsigned int Numberpl; 
			Card generalcarts[5];        
			int opCards;         

			struct PlayerSeat {
				Player* playerPtr = nullptr;
				bool isFolded = false;
				int currentBet = 0;
			};
			PlayerSeat seats[10];
	public:
		Table() {
			Numberpl = 0;
			stage = preflop;
			opCards = 0;
			for (int i = 0; i < 5; i++){
				generalcarts[i].Setnumber(2);
				generalcarts[i].Setsuit(Suit::clubs);
			}
		}
		void sitPlayer(Player& player) {
			if (Numberpl < 10) {
				seats[Numberpl].playerPtr = &player;
				Numberpl++;
			}
			else cout << "Слишком много игроков, нельзя добавить" << endl;
		}

		void nextstage(Deck &deck) {
			switch (stage) {
			case preflop: stage = flop;opCards = 3;
				for (int i = 0; i < 3;i++) {
					generalcarts[i] = deck.dealCard();
				}
				break;
			case flop: stage = tern; opCards = 4;
				generalcarts[opCards - 1] = deck.dealCard(); 
				break;
			case tern:stage = river;opCards = 5;
				generalcarts[opCards-1] = deck.dealCard();
				break;
			case river:stage = showdown;
				break;
			case showdown: cout << "END OF THE GAME" << endl;
			}
		}
		void printTable() const {
			cout << "Сейчас за столом ";
			switch (stage) {
			case preflop:cout << "Префлоп";break;
			case flop:cout << "Флоп";break;
			case tern:cout << "Терн";break;
			case river:cout << "Ривер";break;
			case showdown:cout << "Шоудаун";break;
			}
			cout << ", " << Numberpl << " игроков:" << endl;
			for (int i = 0;i < Numberpl; i++) {
				cout << "Игрок " << (i + 1) << " имеет ";
				seats[i].playerPtr->printhand();
				cout << endl;
			}
			cout << "Карты на столе:"<<endl;
			for (int i = 0; i < opCards;i++){
				cout << i << " ";
				generalcarts[i].printcard();
			}
			cout <<endl<< "Сейчас пот на столе " << pot << endl;
		}

		int getOpCards() const { return opCards; }
		const Card& getGeneralCart(int index) const {
			return generalcarts[index];
		}
	};

	class CombinationCalculator {
	private:
		bool cards[4][15] = {};
		int dignity[15] = {};
		int countofsuit[4] = {};
		Combinations combination = Combinations::OlderCard;

		void reset() {
			fill(&cards[0][0], &cards[0][0] + sizeof(cards) / sizeof(bool), false);
			fill(begin(dignity), end(dignity), 0);
			fill(begin(countofsuit), end(countofsuit), 0);
			combination = Combinations::OlderCard;
		}

		int getSuitIndex(Suit suit) {
			switch (suit) {
			case Suit::clubs:    return 0;
			case Suit::diamonds: return 1;
			case Suit::hearts:   return 2;
			case Suit::spades:   return 3;
			default:             return 0;
			}
		}

		void addCard(const Card& card) {
			int suit = getSuitIndex(card.Getsuit());
			int rank = card.Getnumber();
			cards[suit][rank] = true;
			dignity[rank]++;
			countofsuit[suit]++;
		}

	public:
		CombinationCalculator() { reset(); }

		void gathercomb(Player& player, const Table& table) {
			reset();
			addCard(player.getF());
			addCard(player.getS());
			for (int i = 0; i < table.getOpCards(); i++) {
				addCard(table.getGeneralCart(i));
			}
		}

		void calculateCards() {
			int count4 = 0, count3 = 0, count2 = 0;
			bool isFlush = false;
			int flushSuit = -1;

			for (int i = 0; i < 4; i++) {
				if (countofsuit[i] >= 5) {
					isFlush = true;
					flushSuit = i;
					break;
				}
			}

			for (int i = 14; i >= 2; i--) {
				if (dignity[i] == 4) count4++;
				else if (dignity[i] == 3) count3++;
				else if (dignity[i] == 2) count2++;
			}

			bool isStraight = false;
			bool isStraightFlush = false;
			bool isRoyalFlush = false;

			int straightStreak = 0;
			int straightFlushStreak = 0;

			for (int i = 14; i >= 1; i--) {
				int rank = (i == 1) ? 14 : i;

				if (dignity[rank] > 0) {
					straightStreak++;
					if (straightStreak >= 5) isStraight = true;
				}
				else {
					straightStreak = 0;
				}

				if (isFlush && cards[flushSuit][rank]) {
					straightFlushStreak++;
					if (straightFlushStreak >= 5) {
						isStraightFlush = true;
						if (i == 10) isRoyalFlush = true;
					}
				}
				else {
					straightFlushStreak = 0;
				}
			}

			if (isRoyalFlush) combination = Combinations::Royal_Flash;
			else if (isStraightFlush) combination = Combinations::Straight_Flash;
			else if (count4 > 0) combination = Combinations::FourofKind;
			else if ((count3 > 0 && count2 > 0) || count3 > 1) combination = Combinations::FullHouse;
			else if (isFlush) combination = Combinations::Flush;
			else if (isStraight) combination = Combinations::Straight;
			else if (count3 > 0) combination = Combinations::ThreeofKind;
			else if (count2 >= 2) combination = Combinations::TwoPairs;
			else if (count2 > 0) combination = Combinations::Pair;
			else combination = Combinations::OlderCard;
		}

		Combinations getCombination() const { return combination; }
	};
	class Equity {
	private:
		double chance;
	public:
	};

	int main()
	{
		setlocale(LC_ALL, "ru");

		Deck deck;
		deck.shuffleDeck();

		Card dummy;
		Player player1(dummy, dummy);
		Player player2(dummy, dummy);

		player1.getfromdeck(deck);
		player2.getfromdeck(deck);

	
		Table table; 
		table.sitPlayer(player1);
		table.sitPlayer(player2);

		table.printTable();

		table.nextstage(deck);
		table.printTable();

		table.nextstage(deck);
		table.printTable();

		table.nextstage(deck);
		table.printTable();
		table.nextstage(deck);
		table.nextstage(deck);
		table.printTable();
		return 0;
	}