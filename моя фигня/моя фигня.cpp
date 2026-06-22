	#include <iostream>
	#include <string>
	#include <vector>
	#include <algorithm> 
	#include <random>
 
	using namespace std;

	enum class Suit{ clubs , diamonds, hearts, spades, None};
	enum Stage { beginning, preflop, flop, tern , river, showdown, };
	enum class Combinations {
		OlderCard, Pair, TwoPairs, ThreeofKind, Straight, Flush, FullHouse, FourofKind, Straight_Flash, Royal_Flash
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
		Suit allsuits[4] = { Suit::clubs, Suit::diamonds,Suit::hearts,Suit::spades };
	public:
		Deck() {
			
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
		int PlayerBudget;
		string name;
		bool isFolded = false;
	public:
		Player() {
			setFirstCard(Suit::None, 0);
			setSecondCard(Suit::None, 0);
			PlayerBudget = 0;
		}
		Player(Card &cart1, Card &cart2) {
			Hand[0] = cart1;
			Hand[1] = cart2;
		}

		void setSecondCard(Suit suit , int numer) {
			Hand[1].Setsuit(suit);
			Hand[1].Setnumber(numer);
		}
		void setFirstCard(const Card& cart) { Hand[0] = cart; }
		void setFirstCard(Suit suit, int numer) {
			Hand[0].Setsuit(suit);
			Hand[0].Setnumber(numer);
		}
		void setSecondCard(const Card& cart) { Hand[1] = cart; }

		void printhand() const {
			cout << "У " << name << " на руках : " << endl << "1 карта ";
			Hand[0].printcard();
			cout << "2 карта ";
			Hand[1].printcard();
		}

		void getfromdeck(Card& cart1, Card& cart2) {
			setFirstCard(cart1);
			setSecondCard(cart2);
		}

		const Card& getF() const { return Hand[0]; }
		const Card& getS() const { return Hand[1]; }

		int Raise(int raise) {
			PlayerBudget -= raise;
			return raise;
		}
		bool Fold() {
			isFolded = true;
			return isFolded;
		}
	};
	class Table {
	private:
			int pot = 0;			
			Card generalcarts[5];        
			int opCards;         
			Player seats[10] ;
			
			unsigned int Numberpl;
	public:
		Table() {
			Numberpl = 0;
			opCards = 0;
			for (int i = 0; i < 5; i++){
				generalcarts[i].Setnumber(2);
				generalcarts[i].Setsuit(Suit::clubs);
			}
		}
		void sitPlayer(Player& player) {
			if (Numberpl < 10) {
				seats[Numberpl] = player;
				Numberpl++;
			}
			else cout << "Слишком много игроков, нельзя добавить" << endl;
		}

		void addGeneralCard(Card &card) {
			if (opCards > 5) return;
			generalcarts[opCards - 1] = card;
			opCards++;
		}
		
		/*void printTable() const {
			cout << "Сейчас за столом ";
			
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
		}*/

		int getOpCards() const { return opCards; }
		int getNumofPlayers() const { return Numberpl;  }

		const Card& getGeneralCart(int index) const {
			return generalcarts[index];
		}
		void clearTable(){
			Numberpl = 0;
			opCards = 0;
			for (int i = 0; i < 5; i++) {
				generalcarts[i].Setnumber(0);
				generalcarts[i].Setsuit(Suit::None);
			}
		}
	};

	class CombinationCalculator {
		private:
			bool cards[4][15] = {};
			int dignity[15] = {};
			int countofsuit[4] = {};

			Combinations combination = Combinations::OlderCard;
			vector<int> tieBreakers;

			int count4 = 0, count3 = 0, count2 = 0;
			bool isFlush = false;
			int flushSuit = -1;
			int straightHigh = -1;
			int straightFlushHigh = -1;


			int getSuitIndex(Suit suit) {
				switch (suit) {
				case Suit::clubs:    return 0;
				case Suit::diamonds: return 1;
				case Suit::hearts:   return 2;
				case Suit::spades:   return 3;
				default:             return 0;
				}
			}

			
			void analyzeRawData() {
			
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

			
				int straightStreak = 0;
				int straightFlushStreak = 0;

				for (int i = 14; i >= 1; i--) {
					int rank = (i == 1) ? 14 : i;

					if (dignity[rank] > 0) {
						straightStreak++;
						if (straightStreak >= 5 && straightHigh == -1) {
							straightHigh = (i == 1) ? 5 : (i + 4);
						}
					}
					else {
						straightStreak = 0;
					}

					if (isFlush && cards[flushSuit][rank]) {
						straightFlushStreak++;
						if (straightFlushStreak >= 5 && straightFlushHigh == -1) {
							straightFlushHigh = (i == 1) ? 5 : (i + 4);
						}
					}
					else {
						straightFlushStreak = 0;
					}
				}
			}

			bool tryRoyalFlush() {
				if (straightFlushHigh == 14) {
					combination = Combinations::Royal_Flash;
					tieBreakers = { 14 };
					return true;
				}
				return false;
			}

			bool tryStraightFlush() {
				if (straightFlushHigh != -1) {
					combination = Combinations::Straight_Flash;
					tieBreakers = { straightFlushHigh };
					return true;
				}
				return false;
			}

			bool tryFourOfAKind() {
				if (count4 > 0) {
					combination = Combinations::FourofKind;
					int quads = -1, kicker = -1;
					for (int i = 14; i >= 2; i--) {
						if (dignity[i] == 4) quads = i;
						else if (dignity[i] > 0 && kicker == -1) kicker = i;
					}
					tieBreakers = { quads, kicker };
					return true;
				}
				return false;
			}

			bool tryFullHouse() {
				if ((count3 > 0 && count2 > 0) || count3 > 1) {
					combination = Combinations::FullHouse;
					int trips = -1, pair = -1;
					for (int i = 14; i >= 2; i--) {
						if (dignity[i] == 3) { trips = i; break; }
					}
					for (int i = 14; i >= 2; i--) {
						if (i != trips && dignity[i] >= 2) { pair = i; break; }
					}
					tieBreakers = { trips, pair };
					return true;
				}
				return false;
			}

			bool tryFlush() {
				if (isFlush) {
					combination = Combinations::Flush;
					for (int i = 14; i >= 2; i--) {
						if (cards[flushSuit][i] && tieBreakers.size() < 5) {
							tieBreakers.push_back(i);
						}
					}
					return true;
				}
				return false;
			}

			bool tryStraight() {
				if (straightHigh != -1) {
					combination = Combinations::Straight;
					tieBreakers = { straightHigh };
					return true;
				}
				return false;
			}

			bool tryThreeOfAKind() {
				if (count3 > 0) {
					combination = Combinations::ThreeofKind;
					int trips = -1;
					for (int i = 14; i >= 2; i--) {
						if (dignity[i] == 3) { trips = i; break; }
					}
					tieBreakers.push_back(trips);
					for (int i = 14; i >= 2; i--) {
						if (dignity[i] > 0 && i != trips && tieBreakers.size() < 3) {
							tieBreakers.push_back(i);
						}
					}
					return true;
				}
				return false;
			}

			bool tryTwoPairs() {
				if (count2 >= 2) {
					combination = Combinations::TwoPairs;
					int pair1 = -1, pair2 = -1;
					for (int i = 14; i >= 2; i--) {
						if (dignity[i] == 2) {
							if (pair1 == -1) pair1 = i;
							else if (pair2 == -1) { pair2 = i; break; }
						}
					}
					tieBreakers = { pair1, pair2 };
					for (int i = 14; i >= 2; i--) {
						if (dignity[i] > 0 && i != pair1 && i != pair2) {
							tieBreakers.push_back(i);
							break;
						}
					}
					return true;
				}
				return false;
			}

			bool tryPair() {
				if (count2 > 0) {
					combination = Combinations::Pair;
					int pair = -1;
					for (int i = 14; i >= 2; i--) {
						if (dignity[i] == 2) { pair = i; break; }
					}
					tieBreakers.push_back(pair);
					for (int i = 14; i >= 2; i--) {
						if (dignity[i] > 0 && i != pair && tieBreakers.size() < 4) {
							tieBreakers.push_back(i);
						}
					}
					return true;
				}
				return false;
			}

			void setOlderCard() {
				combination = Combinations::OlderCard;
				for (int i = 14; i >= 2; i--) 
					if (dignity[i] > 0 && tieBreakers.size() < 5) 
						tieBreakers.push_back(i);
			
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
		CombinationCalculator() {
			tieBreakers.reserve(5);
		}
		
		void addCard(const Card& card) {
			int suit = getSuitIndex(card.Getsuit());
			int rank = card.Getnumber();
			cards[suit][rank] = true;
			dignity[rank]++;
			countofsuit[suit]++;
		}

		void reset() {
			memset(cards, 0, sizeof(cards));
			memset(dignity, 0, sizeof(dignity));
			memset(countofsuit, 0, sizeof(countofsuit));
			combination = Combinations::OlderCard;
			tieBreakers.clear();

			count4 = count3 = count2 = 0;
			isFlush = false;
			flushSuit = -1;
			straightHigh = -1;
			straightFlushHigh = -1;
		}

		void evaluate(const Card (&playerHand)[2], const Card (&tableCards)[], const int Opcards) {
			reset(); 
	
			for (int i = 0; i < 2; i++) addCard(playerHand[i]);
			for (int i = 0; i < Opcards; i++) addCard(tableCards[i]);;

			calculateCards(); 
		}
		bool operator==(const CombinationCalculator& other) const {
			return (this->combination == other.combination && this->tieBreakers == other.tieBreakers);
		}

		bool operator!=(const CombinationCalculator& other) const {
			return !(*this == other);
		}

		bool operator>(const CombinationCalculator& other) const {
			if (this->combination != other.combination) {
				return this->combination > other.combination;
			}
			return this->tieBreakers > other.tieBreakers;
		}

		bool operator<(const CombinationCalculator& other) const {
			return other > *this;
		}
	};


	class Game {
	private:
		Table table;
		Stage stage;
		Deck deck;
	public:
		Game() {
			table = Table();
			stage = beginning;
			deck = Deck();
		}
		void addPlayer(Player& player) {
			if (stage == beginning)
				table.sitPlayer(player);
			else
				cout << "Во врея игры нельзя садиться за стол" << endl;
		}
		void  



		/*switch (stage) {
		case preflop:cout << "Префлоп";break;
		case flop:cout << "Флоп";break;
		case tern:cout << "Терн";break;
		case river:cout << "Ривер";break;
		case showdown:cout << "Шоудаун";break;
		}*/
		/*void nextstage() {
			switch (stage) {
			case preflop: stage = flop;table.opCards = 3;
				for (int i = 0; i < 3;i++) {
					table.addGeneralCard(deck.dealCard());
				}
				break;
			case flop: stage = tern; opCards = 4;
				generalcarts[opCards - 1] = deck.dealCard();
				break;
			case tern:stage = river;opCards = 5;
				generalcarts[opCards - 1] = deck.dealCard();
				break;
			case river:stage = showdown;
				break;
			case showdown: cout << "END OF THE GAME" << endl;
			}
		}*/
	};

	int main()
	{
		setlocale(LC_ALL, "ru");

		Deck deck;
		deck.shuffleDeck();

		/*Card dummy;
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
		table.printTable();*/
		return 0;
	}