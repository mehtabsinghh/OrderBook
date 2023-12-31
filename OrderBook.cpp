#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <mutex>

// Order class which contains all details related to the order being placed
class Order {
private:
    std::string type;
    int price;      // Price of the order
    int quantity;   // Quantity of the order
    int orderID;    // Unique order ID

public:
    Order(std::string type, int price, int quantity, int orderID) {
        this->type = type;
        this->price = price;
        this->quantity = quantity;
        this->orderID = orderID;
    }
    ~Order() = default;

    // Getter methods for retrieving order details
    int getPrice() {
        return price;
    }

    int getQuantity() {
        return quantity;
    }

    int getOrderID() {
        return orderID;
    }

    // Method to update the quantity of an order
    void setQuantity(int newQuantity) {
        this->quantity = newQuantity;
    }

    std::string getType() {
        return this->type;
    }

    std::string toJSON() const {
        std::string json = "{";
        json += "\"type\":\"" + type + "\",";
        json += "\"price\":" + std::to_string(price) + ",";
        json += "\"quantity\":" + std::to_string(quantity) + ",";
        json += "\"orderID\":" + std::to_string(orderID);
        json += "}";
        return json;
    }

    // Comparison operators to facilitate ordering in priority queues
    bool operator<(const Order& other) const {
        return other.price < price;
    }

    bool operator>(const Order& other) const {
        return other.price > price;
    }
};

// Manages the data of the order book
class OrderBookData {
private:
    std::priority_queue<Order, std::vector<Order>, std::less<Order> > bestAsk;
    std::priority_queue<Order, std::vector<Order>, std::greater<Order> > bestBid;

public:
    OrderBookData() = default;
    ~OrderBookData() = default;

    void addAskOrder(const Order& askOrder) {
        bestAsk.push(askOrder);
    }

    void addBidOrder(const Order& bidOrder) {
        bestBid.push(bidOrder);
    }

    Order bestAskTop() {
        return bestAsk.top();
    }

    Order bestBidTop() {
        return bestBid.top();
    }

    void bestAskPop() {
        bestAsk.pop();
    }

    void bestBidPop() {
        bestBid.pop();
    }

    bool bestBidEmpty() {
        return bestBid.empty();
    }

    bool bestAskEmpty() {
        return bestAsk.empty();
    }

    std::priority_queue<Order, std::vector<Order>, std::less<Order> > getBestAskQueue() const {
        return bestAsk;
    }

    std::priority_queue<Order, std::vector<Order>, std::greater<Order> > getBestBidQueue() const {
        return bestBid;
    }
};

// Used to serialise and deserialise the orderbook implemented as a singleton class
class SerialisationService {
private:
    static SerialisationService *uniqueInstance;
    static std::mutex mtx;
    std::string filename;

    SerialisationService() {
        this->filename = "orderbook_data.json";
    }
    ~SerialisationService() = default;

public:
    SerialisationService(const SerialisationService &) = delete;
    SerialisationService &operator=(const SerialisationService &) = delete;

    static SerialisationService *getInstance() {
        if (uniqueInstance == nullptr)
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (uniqueInstance == nullptr)
            {
                uniqueInstance = new SerialisationService();
            }
        }
        return uniqueInstance;
    }

    void serialise(const OrderBookData& orderBookData) {
        std::priority_queue<Order, std::vector<Order>, std::less<Order> > tempAsk = orderBookData.getBestAskQueue();
        std::priority_queue<Order, std::vector<Order>, std::greater<Order> > tempBid = orderBookData.getBestBidQueue();
        std::ofstream outFile(filename);
        bool flag = false;

        if (outFile.is_open()) {
            outFile << "[" << std::endl;
            if (!tempAsk.empty()) {
                Order ask = tempAsk.top();
                outFile << ask.toJSON();
                tempAsk.pop();
                flag = true;
            }

            while (!tempAsk.empty()) {
                outFile << "," << std::endl;
                Order ask = tempAsk.top();
                outFile << ask.toJSON();
                tempAsk.pop();
            }


            if (!tempBid.empty()) {
                if (flag) {
                    outFile << "," << std::endl;
                }
                Order bid = tempBid.top();
                outFile << bid.toJSON();
                tempBid.pop();
            }

            while (!tempBid.empty()) {
                outFile << "," << std::endl;
                Order bid = tempBid.top();
                outFile << bid.toJSON();
                tempBid.pop();
            }
            outFile << "\n" << "]" << std::endl;

            
        }
    }

    void deserialise(OrderBookData& orderBookData) {
        std::map<std::string, std::string> attributeMap;
        std::ifstream inFile(filename);
        if (!inFile.is_open()) {
            std::cerr << "Unable to open file: " << filename << std::endl;
            return;
        }

        std::string line;
        std::string content;
        while (std::getline(inFile, line)) {
            content += line;
        }
        inFile.close();

        if (content.empty()) {
            std::cerr << "Empty file or error reading content." << std::endl;
            return;
        }
        size_t pos = 0;

        // Loop to process each JSON-like object in the content
        while ((pos = content.find('{', pos)) != std::string::npos) {
            size_t endPos = content.find('}', pos);
            if (endPos == std::string::npos) {
                break; // Break if no closing bracket found
            }
            std::string orderData = content.substr(pos, endPos - pos + 1);

            // Parse the orderData string to extract key-value pairs
            std::istringstream iss(orderData.substr(1, orderData.length() - 2)); // Remove '{' and '}' from the string
            std::string token;

            while (std::getline(iss, token, ',')) {
                std::istringstream issToken(token);
                std::string key, value;

                std::getline(issToken, key, ':'); // Extract key
                std::getline(issToken, value);    // Extract value

                // Remove quotes and whitespace from key and value
                key.erase(remove_if(key.begin(), key.end(), ::isspace), key.end());
                key.erase(remove(key.begin(), key.end(), '\"'), key.end());
                value.erase(remove_if(value.begin(), value.end(), ::isspace), value.end());
                value.erase(remove(value.begin(), value.end(), '\"'), value.end());

                attributeMap[key] = value; // Store key-value pair in map
            }

            // Accessing attributes using the map
            std::string type = attributeMap["type"];
            int price = std::stoi(attributeMap["price"]);
            int quantity = std::stoi(attributeMap["quantity"]);
            int orderID = std::stoi(attributeMap["orderID"]);
            
            // Create an Order object using the extracted attributes
            Order newOrder(type, price, quantity, orderID);

            // Push the Order object to the appropriate heap based on its type
            if (type == "ASK") {
                orderBookData.addAskOrder(newOrder);
            } else if (type == "BID") {
                orderBookData.addBidOrder(newOrder);
            }

            pos = endPos + 1;
            attributeMap.clear(); // Clear the map for the next object
        }
        std::cout << "Orders deserialized from " << filename << " successfully." << std::endl;
    }
};

SerialisationService *SerialisationService::uniqueInstance = nullptr;
std::mutex SerialisationService::mtx;

// Handles all operations related to the orderbook
class OrderBook {
private:
    SerialisationService *serliaiser;
    OrderBookData orderBookData;
    int orderID = 0;

public:
    OrderBook() {
        this->serliaiser = SerialisationService::getInstance(); 
        serliaiser->deserialise(orderBookData); 
    }
    ~OrderBook() = default;

    // Method to place an ask
    void placeAsk(int price, int quantity) {
        Order ask("ASK", price, quantity, ++orderID);
        orderBookData.addAskOrder(ask);
        serliaiser->serialise(orderBookData);
    }

    // Method to place a bid
    void placeBid(int price, int quantity) {
        Order bid("BID", price, quantity, ++orderID);
        orderBookData.addBidOrder(bid);
        serliaiser->serialise(orderBookData);
    }

    // Method to match bid and ask orders
    void matchBidAsk() {
        while (!orderBookData.bestAskEmpty() && !orderBookData.bestBidEmpty()) {
            Order ask = orderBookData.bestAskTop();
            Order bid = orderBookData.bestBidTop();
            if (ask.getPrice() > bid.getPrice()) {
                // If no orders are eligible for matching, break the loop
                std::cout << "No orders eligible for matching" << std::endl;
                break;
            }
            
            int currentAskQuantity = ask.getQuantity();
            int currentBidQuantity = bid.getQuantity();
            int matchedQuantity = std::min(currentAskQuantity, currentBidQuantity);
            ask.setQuantity(currentAskQuantity - matchedQuantity);
            bid.setQuantity(currentBidQuantity - matchedQuantity);

            // Output matched order details
            std::cout << "Matched: Ask Order ID " << ask.getOrderID() << " with Bid Order ID " << bid.getOrderID()
                      << ", Quantity " << matchedQuantity << ", Price " << ask.getPrice() << std::endl;
                      
            orderBookData.bestAskPop();
            orderBookData.bestBidPop();

            // Push remaining quantities back to respective queues
            if (ask.getQuantity() > 0)
                orderBookData.addAskOrder(ask);
            if (bid.getQuantity() > 0)
                orderBookData.addBidOrder(bid);
        }
        serliaiser->serialise(orderBookData);
    }

    // Method for executing a market buy order
    void marketBuy(int quantity) {
        while (quantity != 0 && !orderBookData.bestAskEmpty()) {
            Order ask = orderBookData.bestAskTop();
            int askQuantity = ask.getQuantity();
            int matchedQuantity = std::min(quantity, askQuantity);
            quantity -= matchedQuantity;
            askQuantity -= matchedQuantity;
            orderBookData.bestAskPop();
            if (askQuantity > 0) {
                // Updates the quantity
                ask.setQuantity(askQuantity);
                orderBookData.addAskOrder(ask);
            }
        }
        serliaiser->serialise(orderBookData);
    }

    // Method for executing a market sell order
    void marketSell(int quantity) {
        while (quantity != 0 && !orderBookData.bestBidEmpty()) {
            Order bid = orderBookData.bestBidTop();
            int bidQuantity = bid.getQuantity();
            int matchedQuantity = std::min(quantity, bidQuantity);
            quantity -= matchedQuantity;
            bidQuantity -= matchedQuantity;
            orderBookData.bestBidPop();
            if (bidQuantity > 0) {
                // Updates the quantity
                bid.setQuantity(bidQuantity);
                orderBookData.addBidOrder(bid);
            }
        }
        serliaiser->serialise(orderBookData);
    }

    // Method to display the current order book
    void displayOrderBook() {
        std::priority_queue<Order, std::vector<Order>, std::less<Order> > tempAsk = orderBookData.getBestAskQueue();
        std::priority_queue<Order, std::vector<Order>, std::greater<Order> > tempBid = orderBookData.getBestBidQueue();

        std::cout << "-----------------------------------------\n";
        std::cout << "\tBid\t\t\tAsk\n";
        std::cout << "-----------------------------------------\n";

        while (!tempBid.empty() || !tempAsk.empty()) {
            if (!tempBid.empty()) {
                Order bid = tempBid.top();
                std::cout << "Price "<< "£" << bid.getPrice() << " " << "Size " << bid.getQuantity() << "\t";
                tempBid.pop();
            } else {
                std::cout << "\t\t";
            }

            if (!tempAsk.empty()) {
                Order ask = tempAsk.top();
                std::cout << "Price "<< "£" << ask.getPrice() << " " << "Size " << ask.getQuantity() << "\n";
                tempAsk.pop();
            } else {
                std::cout << "\n";
            }
        }
        serliaiser->serialise(orderBookData);
    }
};

// Class to let the user interact with the program
class UserInterface {
private:
    OrderBook orderBook;
    OrderBookData orderBookData;

public:
    UserInterface() = default;
    ~UserInterface() = default;

    void display() {
        orderBook.displayOrderBook();
    }

    void run() {
        std::string input;
        display();
        while (true) {
            std::cout << "\nOptions: [bid / ask / exit]\nEnter command: ";
            std::cin >> input;

            if (input == "bid") {
                int price, quantity;
                std::cout << "Enter bid price: ";
                std::cin >> price;
                std::cout << "Enter bid quantity: ";
                std::cin >> quantity;
                orderBook.placeBid(price, quantity);
                std::cout << "Bid placed successfully.\n";
            } else if (input == "ask") {
                int price, quantity;
                std::cout << "Enter ask price: ";
                std::cin >> price;
                std::cout << "Enter ask quantity: ";
                std::cin >> quantity;
                orderBook.placeAsk(price, quantity);
                std::cout << "Ask placed successfully.\n";
            } else if (input == "exit") {
                std::cout << "Exiting the program...\n";
                break;
            } else {
                std::cout << "Invalid command. Please try again.\n";
            }
            orderBook.matchBidAsk();
            display();
        }
    }
};

// Main function for testing the OrderBook functionalities
int main() {
    UserInterface ui;
    ui.run();
    return 0;
}