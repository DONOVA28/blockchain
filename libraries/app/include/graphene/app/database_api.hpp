/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to thew following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <graphene/app/full_account.hpp>

#include <graphene/protocol/types.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/confidential_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/worker_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/fund_object.hpp>
#include <graphene/chain/cheque_object.hpp>

#include <graphene/market_history/market_history_plugin.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace graphene { namespace app {

using namespace graphene::chain;
using namespace graphene::market_history;
using namespace std;

class database_api_impl;

struct order
{
   double                     price;
   double                     quote;
   double                     base;
};

struct order_book
{
  string                      base;
  string                      quote;
  vector< order >             bids;
  vector< order >             asks;
};

struct market_ticker
{
   string                     base;
   string                     quote;
   double                     latest;
   double                     lowest_ask;
   double                     highest_bid;
   double                     percent_change;
   double                     base_volume;
   double                     quote_volume;
};

struct market_volume
{
   string                     base;
   string                     quote;
   double                     base_volume;
   double                     quote_volume;
};

struct market_trade
{
   fc::time_point_sec         date;
   double                     price;
   double                     amount;
   double                     value;
};

struct cheque_info_object
{
   cheque_id_type id;
   fc::time_point_sec datetime_expiration;

   // amount for each payee
   asset payee_amount;

};

struct transfer_fee_info
{
   asset amount;
   std::string name;
   uint32_t precision = 0;
};

struct max_transfer_info
{
   struct fee_t
   {
      asset amount;
      std::string name;
      uint32_t precision = 0;
   };

   asset amount;
   fee_t fee;
};

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
   public:
      database_api(graphene::chain::database& db);
      ~database_api();

      /////////////
      // Objects //
      /////////////

      /**
       * @brief Get the objects corresponding to the provided IDs
       * @param ids IDs of the objects to retrieve
       * @return The objects retrieved, in the order they are mentioned in ids
       *
       * If any of the provided IDs does not map to an object, a null variant is returned in its position.
       */
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      optional<object_id_type> get_last_object_id(object_id_type id) const;

      ///////////////////
      // Subscriptions //
      ///////////////////

      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      /**
       * @brief Stop receiving any notifications
       *
       * This unsubscribes from all subscribed markets and objects.
       */
      void cancel_all_subscriptions();

      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      optional<block_header> get_block_header(uint32_t block_num) const;

      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional<signed_block> get_block(uint32_t block_num) const;

      /**
       * @brief Retrieve a full, signed block
       * @param block_id Id of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional<signed_block> get_block_by_id(string block_id) const;

      /**
       * @brief used to fetch an individual transaction.
       */
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block ) const;

      /**
       * If the transaction has not expired, this method will return the transaction for the given ID or
       * it will return NULL if it is not known.  Just because it is not known does not mean it wasn't
       * included in the blockchain.
       */
      optional<signed_transaction> get_recent_transaction_by_id( const transaction_id_type& id ) const;

      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve the @ref chain_property_object associated with the chain
       */
      chain_property_object get_chain_properties()const;

      /**
       * @brief Retrieve the current @ref global_property_object
       */
      global_property_object get_global_properties()const;

      /**
       * @brief Retrieve compile-time constants
       */
      fc::variant_object get_config()const;

      /**
       * @brief Get the chain ID
       */
      chain_id_type get_chain_id()const;

      /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      dynamic_global_property_object get_dynamic_global_properties()const;

      //////////
      // Keys //
      //////////

      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;

      ///////////////
      // Addresses //
      ///////////////

      vector<vector<account_id_type>> get_address_references( vector<address> addresses )const;
      fc::optional<account_id_type> get_market_reference(const address& key) const;
      address get_address( int64_t block_num, int64_t transaction_num )const;

      //////////////
      // Accounts //
      //////////////

      /**
       * @brief Get a list of accounts by ID
       * @param account_ids IDs of the accounts to retrieve
       * @return The accounts corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids) const;

      /**
       * @brief Returns account addresses
       */
      std::pair<unsigned, vector<address>>
      get_account_addresses(const string& name_or_id, unsigned from, unsigned limit) const;
    
      /**
       * @brief Get list of referrals of account
       */
      Unit get_referrals(const std::string& account_name_or_id);
      ref_info get_referrals2(const std::string& account_name_or_id);
      vector<SimpleUnit> get_accounts_info(vector<string> account_names_or_ids);
      
      /** 
       *  @brief Get count of users in each rank
       *  @return Map of ranks with count of users
       *  { "A": 31,
       *    "B": 3123,
       *    "C": 3,
       *    "D": 32,
       *    "E": 94,
       *    "F": 321,
       *    "G": 0  }
       */
      fc::variant_object get_user_count_by_ranks();

      int64_t get_user_count_with_balances(std::vector<fc::time_point_sec> dates = std::vector<fc::time_point_sec>());

      /**
       * @return number of the last element in query and user ids who have asset 'asst'
       */
      std::pair<uint32_t, std::vector<account_id_type>>
      get_users_with_asset(const asset_id_type& asst, uint32_t start, uint32_t limit) const;

      /**
       * @brief Fetch all objects relevant to the specified accounts and subscribe to updates
       * @param callback Function to call with updates
       * @param names_or_ids Each item must be the name or ID of an account to retrieve
       * @return Map of string from @ref names_or_ids to the corresponding account
       *
       * This function fetches all relevant objects for the given accounts, and subscribes to updates to the given
       * accounts. If any of the strings in @ref names_or_ids cannot be tied to an account, that input will be
       * ignored. All other accounts will be retrieved and subscribed.
       *
       */
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );

      optional<bonus_balances_object> get_bonus_balances( string name_or_id ) const;

      optional<account_object> get_account_by_name( string name )const;

      optional<account_object> get_account_by_vote_id(const vote_id_type& v_id) const;

      /**
       *  @return all accounts that referr to the key or account id in their owner or active authorities.
       */
      vector<account_id_type> get_account_references(account_id_type account_id) const;

      /**
       *  @return restricted account object if exists
       */
      optional<restricted_account_object> get_restricted_account(account_id_type account_id) const;

      /**
       * @brief Get a list of accounts by name
       * @param account_names Names of the accounts to retrieve
       * @return The accounts holding the provided names
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;

      /**
       * @brief Get names and IDs for registered accounts
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of account names to corresponding IDs
       */
      map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

      //////////////
      // Balances //
      //////////////

      /**
       * @brief Get an account's balances in various assets
       * @param id ID of the account to get balances for
       * @param assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
       * @return Balances of the account
       */
      vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;

      /// Semantically equivalent to @ref get_account_balances, but takes a name instead of an ID.
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;

      /** @return all unclaimed balance objects for a set of addresses */
      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;

      vector<asset> get_vested_balances( const vector<balance_id_type>& objs )const;

      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;

      /**
       * @brief Get the total number of accounts registered with the blockchain
       */
      uint64_t get_account_count()const;

      ////////////
      // Assets //
      ////////////

      /**
       * @brief Get a list of assets by ID
       * @param asset_ids IDs of the assets to retrieve
       * @return The assets corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;

      /**
       * @brief Get assets alphabetically by symbol name
       * @param lower_bound_symbol Lower bound of symbol names to retrieve
       * @param limit Maximum number of assets to fetch (must not exceed 100)
       * @return The assets found
       */
      vector<asset_object> list_assets(const string& lower_bound_symbol, uint32_t limit)const;

      /**
       * @brief Get a list of assets by symbol
       * @param asset_symbols Symbols or stringified IDs of the assets to retrieve
       * @return The assets corresponding to the provided symbols or IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      ////////////
      // Funds  //
      ////////////

      /**
       * @brief Get funds alphabetically by id
       * @return The funds found
       */
      vector<fund_object> list_funds() const;

      /**
       * @brief Get fund by id or name
       * @return The fund found
       */
      fund_object get_fund(const std::string& fund_name_or_id) const;

      /**
       * @brief Get fund by its owner's id or name
       * @return The fund found
       */
      fund_object get_fund_by_owner(const std::string& account_name_or_id) const;

      /**
       * @brief Get fund deposits alphabetically by id
       * @param limit Maximum number of deposits to fetch (must not exceed 100)
       * @return The fund deposits found
       */
      vector<fund_deposit_object> get_fund_deposits(const std::string& fund_name_or_id, uint32_t start, uint32_t limit) const;

      /**
       * @brief Get all fund deposits alphabetically by id
       * @param period Period (in days)
       * @param limit Maximum number of deposits to fetch (must not exceed 100)
       * @return the fund deposits found | new start position
       */
      pair<vector<fund_deposit_object>, uint32_t>
      get_all_fund_deposits_by_period(uint32_t period, uint32_t start, uint32_t limit) const;

      /**
       * @brief Get sum of all user's deposits
       * @param fund_id ID of fund
       * @param account_id ID of account
       * @return asset sum of all user's deposits
       */
      asset get_fund_deposits_amount_by_account(fund_id_type fund_id, account_id_type account_id) const;

      /**
       * @brief Get sum of all user's deposits
       * @param account_id ID of account
       */
      vector<fund_deposit_object> get_account_deposits(account_id_type account_id, uint32_t start, uint32_t limit) const;

      /////////////////////
      // Markets / feeds //
      /////////////////////

      /**
       * @brief Get addresses of the market
       * @param account_id market's ID of account
       */
      vector<market_address_object> get_market_addresses(account_id_type account_id, uint32_t start, uint32_t limit) const;

      /**
       * @brief Get limit orders in a given market
       * @param a ID of asset being sold
       * @param b ID of asset being purchased
       * @param limit Maximum number of orders to retrieve
       * @return The limit orders, ordered from least price to greatest
       */
      vector<limit_order_object> get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const;

      /**
       * @brief Get call orders in a given asset
       * @param a ID of asset being called
       * @param limit Maximum number of orders to retrieve
       * @return The call orders, ordered from earliest to be called to latest
       */
      vector<call_order_object> get_call_orders(asset_id_type a, uint32_t limit)const;

      /**
       * @brief Get forced settlement orders in a given asset
       * @param a ID of asset being settled
       * @param limit Maximum number of orders to retrieve
       * @return The settle orders, ordered from earliest settlement date to latest
       */
      vector<force_settlement_object> get_settle_orders(asset_id_type a, uint32_t limit)const;

      map<account_id_type, uint16_t> get_online_info()const;

      /**
       *  @return all open margin positions for a given account id.
       */
      vector<call_order_object> get_margin_positions( const account_id_type& id )const;

      /**
       * @brief Request notification when the active orders in the market between two assets changes
       * @param callback Callback method which is called when the market changes
       * @param a First asset ID
       * @param b Second asset ID
       *
       * Callback will be passed a variant containing a vector<pair<operation, operation_result>>. The vector will
       * contain, in order, the operations which changed the market, and their results.
       */
      void subscribe_to_market(std::function<void(const variant&)> callback,
                   asset_id_type a, asset_id_type b);

      /**
       * @brief Unsubscribe from updates to a given market
       * @param a First asset ID
       * @param b Second asset ID
       */
      void unsubscribe_from_market( asset_id_type a, asset_id_type b );

      /**
       * @brief Returns the ticker for the market assetA:assetB
       * @param a String name of the first asset
       * @param b String name of the second asset
       * @return The market ticker for the past 24 hours.
       */
      market_ticker get_ticker( const string& base, const string& quote )const;

      /**
       * @brief Returns the 24 hour volume for the market assetA:assetB
       * @param a String name of the first asset
       * @param b String name of the second asset
       * @return The market volume over the past 24 hours
       */
      market_volume get_24_volume( const string& base, const string& quote )const;

      /**
       * @brief Returns the order book for the market base:quote
       * @param base String name of the first asset
       * @param quote String name of the second asset
       * @param depth of the order book. Up to depth of each asks and bids, capped at 50. Prioritizes most moderate of each
       * @return Order book of the market
       */
      order_book get_order_book( const string& base, const string& quote, unsigned limit = 50 )const;

      /**
       * @brief Returns recent trades for the market assetA:assetB
       * Note: Currentlt, timezone offsets are not supported. The time must be UTC.
       * @param a String name of the first asset
       * @param b String name of the second asset
       * @param stop Stop time as a UNIX timestamp
       * @param limit Number of trasactions to retrieve, capped at 100
       * @param start Start time as a UNIX timestamp
       * @return Recent transactions in the market
       */
      vector<market_trade> get_trade_history( const string& base, const string& quote, fc::time_point_sec start, fc::time_point_sec stop, unsigned limit = 100 )const;

      ///////////////
      // Witnesses //
      ///////////////

      /**
       * @brief Get a list of witnesses by ID
       * @param witness_ids IDs of the witnesses to retrieve
       * @return The witnesses corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;

      /**
       * @brief Get the witness owned by a given account
       * @param account The ID of the account whose witness should be retrieved
       * @return The witness object, or null if the account does not have a witness
       */
      fc::optional<witness_object> get_witness_by_account(account_id_type account)const;

      /**
       * @brief Get names and IDs for registered witnesses
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of witness names to corresponding IDs
       */
      map<string, witness_id_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;

      /**
       * @brief Get the total number of witnesses registered with the blockchain
       */
      uint64_t get_witness_count()const;

      ///////////////////////
      // Committee members //
      ///////////////////////

      /**
       * @brief Get a list of committee_members by ID
       * @param committee_member_ids IDs of the committee_members to retrieve
       * @return The committee_members corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<committee_member_object>> get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const;

      /**
       * @brief Get the committee_member owned by a given account
       * @param account The ID of the account whose committee_member should be retrieved
       * @return The committee_member object, or null if the account does not have a committee_member
       */
      fc::optional<committee_member_object> get_committee_member_by_account(account_id_type account)const;

      /**
       * @brief Get names and IDs for registered committee_members
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of committee_member names to corresponding IDs
       */
      map<string, committee_member_id_type> lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const;


      /// WORKERS

      /**
       * Return the worker objects associated with this account.
       */
      vector<worker_object> get_workers_by_account(account_id_type account)const;


      ///////////
      // Votes //
      ///////////

      /**
       *  @brief Given a set of votes, return the objects they are voting for.
       *
       *  This will be a mixture of committee_member_object, witness_objects, and worker_objects
       *
       *  The results will be in the same order as the votes.  Null will be returned for
       *  any vote ids that are not found.
       */
      vector<variant> lookup_vote_ids(const vector<vote_id_type>& votes) const;

      /**
       *  @brief Given a set of accounts, who voted with specified votes.
       */
      vector<account_id_type> get_voting_accounts(const vote_id_type& vote) const;

      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string get_transaction_hex(const signed_transaction& trx)const;

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      set<address> get_potential_address_signatures( const signed_transaction& trx )const;
      fee_schedule get_current_fee_schedule() const;

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      bool           verify_authority( const signed_transaction& trx )const;

      /**
       * @return true if the signers have enough authority to authorize an account
       */
      bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      /**
       *  Validates a transaction against the current state without broadcasting it on the network.
       */
      processed_transaction validate_transaction( const signed_transaction& trx )const;

      /**
       *  For each operation calculate the required fee in the specified asset type.  If the asset type does
       *  not have a valid core_exchange_rate
       */
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;

      ///////////////////////////
      // Proposed transactions //
      ///////////////////////////

      /**
       *  @return the set of proposed transactions relevant to the specified account id.
       */
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;

      //////////////////////
      // Blinded balances //
      //////////////////////

      /**
       *  @return the set of blinded balance objects by commitment ID
       */
      vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;

      //////////////////////
      //     Cheques      //
      //////////////////////

      optional<cheque_info_object> get_cheque_by_code(const std::string& code) const;

      //////////////////////
      //     Others       //
      //////////////////////

      optional<signed_block> get_block_reserved(uint32_t block_num) const;

      transfer_fee_info get_required_transfer_fee(const asset& amount, account_id_type from, account_id_type to) const;
      transfer_fee_info get_required_blind_transfer_fee(const asset& amount, account_id_type from, account_id_type to) const;
      transfer_fee_info get_required_cheque_fee(const asset& amount, uint32_t count, account_id_type acc_id) const;

      max_transfer_info get_max_transfer_amount_and_fee(const asset& amount, bool is_blind, account_id_type from, account_id_type to) const;
      max_transfer_info get_max_cheque_amount_and_fee(const asset& amount, account_id_type acc_id) const;
      asset get_burnt_asset(asset_id_type id) const;

private:
      std::shared_ptr<database_api_impl> my;

}; // database_api

} }

FC_REFLECT( graphene::app::order, (price)(quote)(base) )
FC_REFLECT( graphene::app::order_book, (base)(quote)(bids)(asks) )
FC_REFLECT( graphene::app::market_ticker, (base)(quote)(latest)(lowest_ask)(highest_bid)(percent_change)(base_volume)(quote_volume) )
FC_REFLECT( graphene::app::market_volume, (base)(quote)(base_volume)(quote_volume) )
FC_REFLECT( graphene::app::market_trade, (date)(price)(amount)(value) )
FC_REFLECT( graphene::app::cheque_info_object,
            (id)
            (datetime_expiration)
            (payee_amount)
          )
FC_REFLECT(graphene::app::transfer_fee_info, (amount)(name)(precision))

FC_REFLECT(graphene::app::max_transfer_info::fee_t, (amount)(name)(precision))
FC_REFLECT(graphene::app::max_transfer_info, (amount)(fee))

extern template class fc::api<graphene::app::database_api>;

FC_API(graphene::app::database_api,
   // Objects
   (get_objects)
   (get_last_object_id)

   // Subscriptions
   (set_subscribe_callback)
   (set_pending_transaction_callback)
   (set_block_applied_callback)
   (cancel_all_subscriptions)

   // Blocks and transactions
   (get_block_header)
   (get_block_by_id)
   (get_block)
   (get_transaction)
   (get_recent_transaction_by_id)
   (get_block_reserved)

   // Globals
   (get_chain_properties)
   (get_global_properties)
   (get_config)
   (get_chain_id)
   (get_dynamic_global_properties)

   // Keys
   (get_key_references)

   // Adresses
   (get_address_references)
   (get_market_reference)
   (get_address)

   // Accounts
   (get_accounts)
   (get_account_addresses)
   (get_referrals)
   (get_referrals2)
   (get_accounts_info)
   (get_user_count_by_ranks)
   (get_user_count_with_balances)
   (get_users_with_asset)
   (get_full_accounts)
   (get_bonus_balances)
   (get_account_by_name)
   (get_account_by_vote_id)
   (get_account_references)
   (get_restricted_account)
   (lookup_account_names)
   (lookup_accounts)
   (get_account_count)

   // Balances
   (get_account_balances)
   (get_named_account_balances)
   (get_balance_objects)
   (get_vested_balances)
   (get_vesting_balances)

   // Assets
   (get_assets)
   (list_assets)
   (lookup_asset_symbols)

   // Funds
   (list_funds)
   (get_fund)
   (get_fund_by_owner)
   (get_fund_deposits)
   (get_all_fund_deposits_by_period)
   (get_fund_deposits_amount_by_account)
   (get_account_deposits)

   // Markets / feeds
   (get_market_addresses)
   (get_order_book)
   (get_limit_orders)
   (get_call_orders)
   (get_settle_orders)
   (get_online_info)
   (get_margin_positions)
   (subscribe_to_market)
   (unsubscribe_from_market)
   (get_ticker)
   (get_24_volume)
   (get_trade_history)

   // Witnesses
   (get_witnesses)
   (get_witness_by_account)
   (lookup_witness_accounts)
   (get_witness_count)

   // Committee members
   (get_committee_members)
   (get_committee_member_by_account)
   (lookup_committee_member_accounts)

   // workers
   (get_workers_by_account)
   // Votes
   (lookup_vote_ids)
   (get_voting_accounts)

   // Authority / validation
   (get_transaction_hex)
   (get_required_signatures)
   (get_potential_signatures)
   (get_potential_address_signatures)
   (get_current_fee_schedule)
   (verify_authority)
   (verify_account_authority)
   (validate_transaction)
   (get_required_fees)

   // Proposed transactions
   (get_proposed_transactions)

   // Blinded balances
   (get_blinded_balances)

   // cheques
   (get_cheque_by_code)

   // others
   (get_required_transfer_fee)
   (get_required_blind_transfer_fee)
   (get_required_cheque_fee)
   (get_max_transfer_amount_and_fee)
   (get_max_cheque_amount_and_fee)
   (get_burnt_asset)
)