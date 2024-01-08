#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>

#include <atomicassets-interface.hpp>
#include <delphioracle-interface.hpp>

using namespace std;
using namespace eosio;

static constexpr name DEFAULT_MARKETPLACE_CREATOR = name("fees.atomic");


/**
* This function takes a vector of asset ids, sorts them and then returns the sha256 hash
* It should therefore return the same hash for two vectors if and only if both vectors include
* exactly the same asset ids in any order
*/
checksum256 hash_asset_ids(const vector <uint64_t> &asset_ids) {
    uint64_t asset_ids_array[asset_ids.size()];
    std::copy(asset_ids.begin(), asset_ids.end(), asset_ids_array);
    std::sort(asset_ids_array, asset_ids_array + asset_ids.size());

    return eosio::sha256((char *) asset_ids_array, sizeof(asset_ids_array));
};


CONTRACT atomicmarket : public contract {
public:
    using contract::contract;

    ACTION init();

    ACTION convcounters();

    ACTION setminbidinc(
        double minimum_bid_increase
    );

    ACTION setversion(
        string new_version
    );

    ACTION addconftoken(
        name token_contract,
        symbol token_symbol
    );

    ACTION adddelphi(
        name delphi_pair_name,
        bool invert_delphi_pair,
        symbol listing_symbol,
        symbol settlement_symbol
    );

    ACTION setmarketfee(
        double maker_market_fee,
        double taker_market_fee
    );

    ACTION addbonusfee(
        name fee_recipient,
        double fee,
        vector <name> applicable_counter_names,
        string fee_name
    );

    ACTION addafeectr(
        uint64_t bonusfee_id,
        name counter_name_to_add
    );

    ACTION stopbonusfee(
        uint64_t bonusfee_id
    );

    ACTION delbonusfee(
        uint64_t bonusfee_id
    );


    ACTION regmarket(
        name creator,
        name marketplace_name
    );


    ACTION withdraw(
        name owner,
        asset token_to_withdraw
    );


    ACTION announcesale(
        name seller,
        vector <uint64_t> asset_ids,
        asset listing_price,
        symbol settlement_symbol,
        name maker_marketplace
    );

    ACTION cancelsale(
        uint64_t sale_id
    );

    ACTION purchasesale(
        name buyer,
        uint64_t sale_id,
        uint64_t intended_delphi_median,
        name taker_marketplace
    );

    ACTION assertsale(
        uint64_t sale_id,
        vector <uint64_t> asset_ids_to_assert,
        asset listing_price_to_assert,
        symbol settlement_symbol_to_assert
    );


    ACTION announceauct(
        name seller,
        vector <uint64_t> asset_ids,
        asset starting_bid,
        uint32_t duration,
        name maker_marketplace
    );

    ACTION cancelauct(
        uint64_t auction_id
    );

    ACTION auctionbid(
        name bidder,
        uint64_t auction_id,
        asset bid,
        name taker_marketplace
    );

    ACTION auctclaimbuy(
        uint64_t auction_id
    );

    ACTION auctclaimsel(
        uint64_t auction_id
    );

    ACTION assertauct(
        uint64_t auction_id,
        vector <uint64_t> asset_ids_to_assert
    );


    ACTION createbuyo(
        name buyer,
        name recipient,
        asset price,
        vector <uint64_t> asset_ids,
        string memo,
        name maker_marketplace
    );

    ACTION cancelbuyo(
        uint64_t buyoffer_id
    );

    ACTION acceptbuyo(
        uint64_t buyoffer_id,
        vector <uint64_t> expected_asset_ids,
        asset expected_price,
        name taker_marketplace
    );

    ACTION declinebuyo(
        uint64_t buyoffer_id,
        string decline_memo
    );


    ACTION paysaleram(
        name payer,
        uint64_t sale_id
    );

    ACTION payauctram(
        name payer,
        uint64_t auction_id
    );

    ACTION paybuyoram(
        name payer,
        uint64_t buyoffer_id
    );


    [[eosio::on_notify("*::transfer")]]
    void receive_token_transfer(
        name from,
        name to,
        asset quantity,
        string memo
    );

    [[eosio::on_notify("atomicassets::transfer")]]
    void receive_asset_transfer(
        name from,
        name to,
        vector <uint64_t> asset_ids,
        string memo
    );

    [[eosio::on_notify("atomicassets::lognewoffer")]]
    void receive_asset_offer(
        uint64_t offer_id,
        name sender,
        name recipient,
        vector <uint64_t> sender_asset_ids,
        vector <uint64_t> recipient_asset_ids,
        string memo
    );

    ACTION lognewsale(
        uint64_t sale_id,
        name seller,
        vector <uint64_t> asset_ids,
        asset listing_price,
        symbol settlement_symbol,
        name maker_marketplace,
        name collection_name,
        double collection_fee
    );

    ACTION lognewauct(
        uint64_t auction_id,
        name seller,
        vector <uint64_t> asset_ids,
        asset starting_bid,
        uint32_t duration,
        uint32_t end_time,
        name maker_marketplace,
        name collection_name,
        double collection_fee
    );

    ACTION lognewbuyo(
        uint64_t buyoffer_id,
        name buyer,
        name recipient,
        asset price,
        vector <uint64_t> asset_ids,
        string memo,
        name maker_marketplace,
        name collection_name,
        double collection_fee
    );

    ACTION logsalestart(
        uint64_t sale_id,
        uint64_t offer_id
    );

    ACTION logauctstart(
        uint64_t auction_id
    );

private:
    struct COUNTER_RANGE {
        name counter_name;
        uint64_t start_id;
        uint64_t end_id;
    };

    struct TOKEN {
        name   token_contract;
        symbol token_symbol;
    };

    struct SYMBOLPAIR {
        symbol listing_symbol;
        symbol settlement_symbol;
        name   delphi_pair_name;
        bool   invert_delphi_pair;
    };


    TABLE balances_s {
        name           owner;
        vector <asset> quantities;

        uint64_t primary_key() const { return owner.value; };
    };

    typedef multi_index <name("balances"), balances_s> balances_t;


    TABLE sales_s {
        uint64_t          sale_id;
        name              seller;
        vector <uint64_t> asset_ids;
        int64_t           offer_id; //-1 if no offer has been created yet, else the offer id
        asset             listing_price;
        symbol            settlement_symbol;
        name              maker_marketplace;
        name              collection_name;
        double            collection_fee;

        uint64_t primary_key() const { return sale_id; };

        checksum256 asset_ids_hash() const { return hash_asset_ids(asset_ids); };
    };

    typedef multi_index <name("sales"), sales_s,
        indexed_by < name("assetidshash"), const_mem_fun < sales_s, checksum256, &sales_s::asset_ids_hash>>>
    sales_t;


    TABLE auctions_s {
        uint64_t          auction_id;
        name              seller;
        vector <uint64_t> asset_ids;
        uint32_t          end_time;   //seconds since epoch
        bool              assets_transferred;
        asset             current_bid;
        name              current_bidder;
        bool              claimed_by_seller;
        bool              claimed_by_buyer;
        name              maker_marketplace;
        name              taker_marketplace;
        name              collection_name;
        double            collection_fee;

        uint64_t primary_key() const { return auction_id; };

        checksum256 asset_ids_hash() const { return hash_asset_ids(asset_ids); };
    };

    typedef multi_index <name("auctions"), auctions_s,
        indexed_by < name("assetidshash"), const_mem_fun < auctions_s, checksum256, &auctions_s::asset_ids_hash>>>
    auctions_t;


    TABLE buyoffers_s {
        uint64_t          buyoffer_id;
        name              buyer;
        name              recipient;
        asset             price;
        vector <uint64_t> asset_ids;
        string            memo;
        name              maker_marketplace;
        name              collection_name;
        double            collection_fee;

        uint64_t primary_key() const { return buyoffer_id; };
    };

    typedef multi_index <name("buyoffers"), buyoffers_s> buyoffers_t;


    TABLE marketplaces_s {
        name marketplace_name;
        name creator;

        uint64_t primary_key() const { return marketplace_name.value; };
    };

    typedef multi_index <name("marketplaces"), marketplaces_s> marketplaces_t;


    TABLE counters_s {
        name     counter_name;
        uint64_t counter_value;

        uint64_t primary_key() const { return counter_name.value; };
    };

    typedef multi_index <name("counters"), counters_s> counters_t;


    TABLE bonusfees_s {
        uint64_t               bonusfee_id;
        name                   fee_recipient;
        double                 fee;
        vector <COUNTER_RANGE> counter_ranges;
        string                 fee_name;

        uint64_t primary_key() const { return bonusfee_id; };
    };

    typedef multi_index <name("bonusfees"), bonusfees_s> bonusfees_t;


    TABLE config_s {
        string              version                  = "1.3.3";
        uint64_t            sale_counter             = 0; // deprecated and no longer used
        uint64_t            auction_counter          = 0; // deprecated and no longer used
        double              minimum_bid_increase     = 0.1;
        uint32_t            minimum_auction_duration = 120; //2 minutes
        uint32_t            maximum_auction_duration = 2592000; //30 days
        uint32_t            auction_reset_duration   = 120; //2 minutes
        vector <TOKEN>      supported_tokens         = {};
        vector <SYMBOLPAIR> supported_symbol_pairs   = {};
        double              maker_market_fee         = 0.01;
        double              taker_market_fee         = 0.01;
        name                atomicassets_account     = atomicassets::ATOMICASSETS_ACCOUNT;
        name                delphioracle_account     = delphioracle::DELPHIORACLE_ACCOUNT;
    };
    typedef singleton <name("config"), config_s>               config_t;
    // https://github.com/EOSIO/eosio.cdt/issues/280
    typedef multi_index <name("config"), config_s>             config_t_for_abi;


    sales_t        sales        = sales_t(get_self(), get_self().value);
    auctions_t     auctions     = auctions_t(get_self(), get_self().value);
    buyoffers_t    buyoffers    = buyoffers_t(get_self(), get_self().value);
    balances_t     balances     = balances_t(get_self(), get_self().value);
    marketplaces_t marketplaces = marketplaces_t(get_self(), get_self().value);
    counters_t     counters     = counters_t(get_self(), get_self().value);
    bonusfees_t    bonusfees    = bonusfees_t(get_self(), get_self().value);
    config_t       config       = config_t(get_self(), get_self().value);


    name get_collection_and_check_assets(name owner, vector <uint64_t> asset_ids);

    name get_collection_author(name collection_name);

    double get_collection_fee(name collection_name);


    uint64_t consume_counter(name counter_name);


    name require_get_supported_token_contract(symbol token_symbol);

    SYMBOLPAIR require_get_symbol_pair(symbol listing_symbol, symbol settlement_symbol);


    bool is_token_supported(name token_contract, symbol token_symbol);

    bool is_symbol_supported(symbol token_symbol);

    bool is_symbol_pair_supported(symbol listing_symbol, symbol settlement_symbol);

    bool is_valid_marketplace(name marketplace);


    void internal_withdraw_tokens(
        name withdrawer,
        asset quantity,
        string memo
    );

    void internal_payout_sale(
        asset quantity,
        name seller,
        name maker_marketplace,
        name taker_marketplace,
        name collection_author,
        double collection_fee,
        name relevant_counter_name,
        uint64_t relevant_counter_id,
        string seller_payout_message
    );

    void internal_add_balance(name owner, asset quantity);

    void internal_decrease_balance(name owner, asset quantity);

    void internal_transfer_assets(name to, vector <uint64_t> asset_ids, string memo);

};