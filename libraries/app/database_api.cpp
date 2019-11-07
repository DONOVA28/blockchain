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
 * furnished to do so, subject to the following conditions:
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

#include <graphene/app/database_api.hpp>
#include <graphene/chain/get_config.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>
#include <map>
#include <future>
#include <iostream>
#include <utility>
#define GET_REQUIRED_FEES_MAX_RECURSION 4

namespace graphene { namespace app {

class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
   public:
      explicit database_api_impl( graphene::chain::database& db );
      ~database_api_impl();

      // Objects
      fc::variants get_objects(const vector<object_id_type>& ids)const;
      optional<object_id_type> get_last_object_id(object_id_type id) const;

      // Subscriptions
      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      void cancel_all_subscriptions();

      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      optional<signed_block> get_block(uint32_t block_num);
      optional<signed_block> get_block_by_id(string block_num);
      void clear_ops(std::vector<operation>& ops);
      processed_transaction get_transaction(uint32_t block_num, uint32_t trx_in_block);
      optional<signed_block> get_block_reserved(uint32_t block_num);

      // Globals
      chain_property_object get_chain_properties() const;
      global_property_object get_global_properties() const;
      fc::variant_object get_config() const;
      chain_id_type get_chain_id() const;
      dynamic_global_property_object get_dynamic_global_properties() const;

      // Keys
      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key ) const;

      // Addresses
      vector<vector<account_id_type>> get_address_references( vector<address> key ) const;

      // market addresses
      fc::optional<account_id_type> get_market_reference(const address& key) const;

      // Accounts
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids) const;

      std::pair<unsigned, vector<address>>
      get_account_addresses(const string& name_or_id, unsigned from, unsigned limit) const;

      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );
      optional<bonus_balances_object> get_bonus_balances ( string name_or_id ) const;
      optional<account_object> get_account_by_name( string name ) const;
      optional<account_object> get_account_by_name_or_id(const string& name_or_id) const;
      optional<account_object> get_account_by_vote_id(const vote_id_type& v_id) const;
      Unit get_referrals( optional<account_object> account ) const;
      ref_info get_referrals_by_id( optional<account_object> account ) const;
      vector<SimpleUnit> get_accounts_info(vector<optional<account_object>> accounts);
      fc::variant_object get_user_count_by_ranks() const;
      int64_t get_user_count_with_balances(fc::time_point_sec start, fc::time_point_sec end) const;
      std::pair<uint32_t, std::vector<account_id_type>> get_users_with_asset(const asset_id_type& asst, uint32_t start, uint32_t limit) const;
      vector<account_id_type> get_account_references(account_id_type account_id) const;
      optional<restricted_account_object> get_restricted_account(account_id_type account_id) const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;

      // Balances
      vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;
      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;
      vector<asset> get_vested_balances( const vector<balance_id_type>& objs )const;
      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;

      // Assets
      vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;
      vector<asset_object>           list_assets(const string& lower_bound_symbol, uint32_t limit)const;
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      // Funds
      vector<fund_object>             list_funds() const;
      const fund_object*              get_fund_by_name_or_id(const std::string& fund_name_or_id) const;
      const fund_object               get_fund(const std::string& fund_name_or_id) const;
      fund_object                     get_fund_by_owner(const std::string& account_name_or_id) const;
      vector<fund_deposit_object>     get_fund_deposits(const std::string& fund_name_or_id, uint32_t start, uint32_t limit) const;
      pair<vector<fund_deposit_object>, uint32_t>
                                      get_all_fund_deposits_by_period(uint32_t period, uint32_t start, uint32_t limit) const;
      asset                           get_fund_deposits_amount_by_account(fund_id_type fund_id, account_id_type account_id) const;
      vector<fund_deposit_object>     get_account_deposits(account_id_type account_id, uint32_t start, uint32_t limit) const;
      vector<market_address_object>   get_market_addresses(account_id_type account_id, uint32_t start, uint32_t limit) const;

      // Markets / feeds
      vector<limit_order_object>      get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const;
      vector<call_order_object>       get_call_orders(asset_id_type a, uint32_t limit)const;
      vector<force_settlement_object> get_settle_orders(asset_id_type a, uint32_t limit)const;
      map<account_id_type, uint16_t>  get_online_info()const;
      vector<call_order_object>       get_margin_positions( const account_id_type& id )const;
      void subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b);
      void unsubscribe_from_market(asset_id_type a, asset_id_type b);
      market_ticker                   get_ticker( const string& base, const string& quote )const;
      market_volume                   get_24_volume( const string& base, const string& quote )const;
      order_book                      get_order_book( const string& base, const string& quote, unsigned limit = 50 )const;
      vector<market_trade>            get_trade_history( const string& base, const string& quote, fc::time_point_sec start, fc::time_point_sec stop, unsigned limit = 100 )const;

      // Witnesses
      vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;
      fc::optional<witness_object> get_witness_by_account(account_id_type account)const;
      map<string, witness_id_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_witness_count()const;

      // Committee members
      vector<optional<committee_member_object>> get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const;
      fc::optional<committee_member_object> get_committee_member_by_account(account_id_type account)const;
      map<string, committee_member_id_type> lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const;

      // Votes
      vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;
      vector<account_id_type> get_voting_accounts(const vote_id_type& vote) const;

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      set<address> get_potential_address_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;
      processed_transaction validate_transaction( const signed_transaction& trx )const;
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;

      // Proposed transactions
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;

      // Blinded balances
      vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;

      optional<cheque_info_object> get_cheque_by_code(const std::string& code) const;

   //private:
      template<typename T>
      void subscribe_to_item( const T& i )const
      {
         auto vec = fc::raw::pack(i);
         if( !_subscribe_callback )
            return;

         if( !is_subscribed_to_item(i) )
         {
            idump((i));
            _subscribe_filter.insert( vec.data(), vec.size() );//(vecconst char*)&i, sizeof(i) );
         }
      }

      template<typename T>
      bool is_subscribed_to_item( const T& i )const
      {
         if( !_subscribe_callback )
            return false;
         return true;
         return _subscribe_filter.contains( i );
      }

      void broadcast_updates( const vector<variant>& updates );

      /** called every time a block is applied to report the objects that were changed */
      void on_objects_changed(const vector<object_id_type>& ids);
      void on_objects_removed(const vector<const object*>& objs);
      void on_applied_block();

      mutable fc::bloom_filter                               _subscribe_filter;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      boost::signals2::scoped_connection _change_connection;
      boost::signals2::scoped_connection _removed_connection;
      boost::signals2::scoped_connection _applied_block_connection;
      boost::signals2::scoped_connection _pending_trx_connection;
      map<pair<asset_id_type,asset_id_type>, std::function<void(const variant&)>> _market_subscriptions;
      graphene::chain::database&                                                  _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api( graphene::chain::database& db )
   : my( new database_api_impl( db ) ) {}

database_api::~database_api() {}

database_api_impl::database_api_impl( graphene::chain::database& db ):_db(db)
{
   wlog("creating database api ${x}", ("x",int64_t(this)) );
   _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids) {
                                on_objects_changed(ids);
                                });
   _removed_connection = _db.removed_objects.connect([this](const vector<const object*>& objs) {
                                on_objects_removed(objs);
                                });
   _applied_block_connection = _db.applied_block.connect([this](const signed_block&){ on_applied_block(); });

   _pending_trx_connection = _db.on_pending_transaction.connect([this](const signed_transaction& trx ){
                         if( _pending_trx_callback ) _pending_trx_callback( fc::variant(trx, GRAPHENE_MAX_NESTED_OBJECTS) );
                      });
}

database_api_impl::~database_api_impl()
{
   elog("freeing database api ${x}", ("x",int64_t(this)) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Objects                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

fc::variants database_api::get_objects(const vector<object_id_type>& ids) const {
   return my->get_objects( ids );
}

fc::variants database_api_impl::get_objects(const vector<object_id_type>& ids) const
{
   if (_subscribe_callback)
   {
      for (auto id: ids)
      {
         if (id.type() == operation_history_object_type && id.space() == protocol_ids) continue;
         if (id.type() == impl_account_transaction_history_object_type && id.space() == implementation_ids) continue;

         this->subscribe_to_item( id );
      }
   }
//   else {
//      elog( "getObjects without subscribe callback??" );
//   }

   fc::variants result;
   result.reserve(ids.size());

   std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                  [this](object_id_type id) -> fc::variant
                  {
                     // exceptions
                     if ((id.space() == implementation_ids) && (id.type() == impl_blind_transfer2_object_type)) return { };
                     if ((id.space() == protocol_ids) && (id.type() == cheque_object_type)) return { };

                     if (auto obj = _db.find_object(id))
                     {
                        if ((id.space() == protocol_ids) && (id.type() == operation_history_object_type))
                        {
                           operation_history_object obj2 = _db.get<operation_history_object>(id);

                           if (obj2.op.which() == operation::tag<blind_transfer2_operation>::value) {
                              obj2.op = blind_transfer2_operation();
                           }
                           else if (obj2.op.which() == operation::tag<cheque_create_operation>::value) {
                              obj2.op.get<cheque_create_operation>().code.clear();
                           }
                           else if (obj2.op.which() == operation::tag<cheque_use_operation>::value) {
                              obj2.op.get<cheque_use_operation>().code.clear();
                           }
                           return obj2.to_variant();
                        }
                        return obj->to_variant();
                     }
                     return { };
                  });

   return result;
}

optional<object_id_type> database_api::get_last_object_id(object_id_type id) const {
   return my->get_last_object_id(id);
}

optional<object_id_type> database_api_impl::get_last_object_id(object_id_type id) const
{
   optional<object_id_type> result_id;

   if (id.type() == operation_history_object_type)
   {
      if (_db.get_index_type<operation_history_index>().indices().size() > 0)
      {
         auto idx = --_db.get_index_type<operation_history_index>().indices().get<by_id>().end();
         result_id = idx->get_id();
      }
   }
   else if (id.type() == account_object_type)
   {
      if (_db.get_index_type<account_index>().indices().size() > 0)
      {
         auto idx = --_db.get_index_type<account_index>().indices().get<by_id>().end();
         result_id = idx->get_id();
      }
   }
   // ...

   return result_id;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_subscribe_callback(std::function<void(const variant&)> cb, bool clear_filter) {
   my->set_subscribe_callback( cb, clear_filter );
}

void database_api_impl::set_subscribe_callback(std::function<void(const variant&)> cb, bool clear_filter)
{
   edump((clear_filter));
   _subscribe_callback = cb;

   if (clear_filter || !cb)
   {
      static fc::bloom_parameters param;
      param.projected_element_count    = 10000;
      param.false_positive_probability = 1.0/10000;
      param.maximum_size = 1024*8*8*2;
      param.compute_optimal_parameters();
      _subscribe_filter = fc::bloom_filter(param);
   }
}

void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   my->set_pending_transaction_callback( cb );
}

void database_api_impl::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   _pending_trx_callback = cb;
}

void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   my->set_block_applied_callback( cb );
}

void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   _block_applied_callback = cb;
}

void database_api::cancel_all_subscriptions()
{
   my->cancel_all_subscriptions();
}

void database_api_impl::cancel_all_subscriptions()
{
   set_subscribe_callback( std::function<void(const fc::variant&)>(), true);
   _market_subscriptions.clear();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num) const {
   return my->get_block_header( block_num );
}

optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
{
   auto result = _db.fetch_block_by_number(block_num);
   if (result) {
      return *result;
   }

   return { };
}

optional<signed_block> database_api::get_block(uint32_t block_num) const {
   return my->get_block( block_num );
}

void database_api_impl::clear_ops(std::vector<operation>& ops)
{
   for (operation& op: ops)
   {
      if (op.which() == operation::tag<blind_transfer2_operation>::value) {
         op = blind_transfer2_operation();
      }
      else if (op.which() == operation::tag<cheque_create_operation>::value) {
         op.get<cheque_create_operation>().code.clear();
      }
      else if (op.which() == operation::tag<cheque_use_operation>::value) {
         op.get<cheque_use_operation>().code.clear();
      }
   }
}

optional<signed_block> database_api_impl::get_block(uint32_t block_num)
{
   auto block = _db.fetch_block_by_number(block_num);

   FC_ASSERT(block.valid(), "invalid block");
   block->update();

   signed_block b = *block;
   for (processed_transaction& tr: b.transactions) {
      clear_ops(tr.operations);
   }
   return b;
}

optional<signed_block> database_api::get_block_reserved(uint32_t block_num) const {
   return my->get_block_reserved(block_num);
}

optional<signed_block> database_api_impl::get_block_reserved(uint32_t block_num)
{
   auto block = _db.fetch_block_by_number(block_num);

   FC_ASSERT(block.valid(), "invalid block");
   block->update();

   return *block;
}

optional<signed_block> database_api::get_block_by_id(string block_id) const {
   return my->get_block_by_id( block_id );
}

optional<signed_block> database_api_impl::get_block_by_id(string block_id)
{
   auto block = _db.fetch_block_by_id( fc::ripemd160(block_id) );
   FC_ASSERT(block.valid(), "invalid block");
   block->update();

   signed_block b = *block;
   for (processed_transaction& tr: b.transactions) {
      clear_ops(tr.operations);
   }

   return b;
}

processed_transaction database_api::get_transaction(uint32_t block_num, uint32_t trx_in_block) const {
   return my->get_transaction( block_num, trx_in_block );
}

optional<signed_transaction> database_api::get_recent_transaction_by_id( const transaction_id_type& id )const
{
   try
   {
      signed_transaction tr = my->_db.get_recent_transaction(id);
      my->clear_ops(tr.operations);

      return tr;
   } catch ( ... ) {
      return optional<signed_transaction>();
   }
}

processed_transaction database_api_impl::get_transaction(uint32_t block_num, uint32_t trx_num)
{
   auto opt_block = _db.fetch_block_by_number(block_num);
   FC_ASSERT( opt_block );
   FC_ASSERT( opt_block->transactions.size() > trx_num );

   processed_transaction& tr = opt_block->transactions[trx_num];
   clear_ops(tr.operations);

   return tr;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

chain_property_object database_api::get_chain_properties()const
{
   return my->get_chain_properties();
}

chain_property_object database_api_impl::get_chain_properties()const
{
   return _db.get(chain_property_id_type());
}

global_property_object database_api::get_global_properties()const
{
   return my->get_global_properties();
}

global_property_object database_api_impl::get_global_properties()const
{
   return _db.get(global_property_id_type());
}

fc::variant_object database_api::get_config()const
{
   return my->get_config();
}

fc::variant_object database_api_impl::get_config()const
{
   return graphene::chain::get_config();
}

chain_id_type database_api::get_chain_id()const
{
   return my->get_chain_id();
}

chain_id_type database_api_impl::get_chain_id()const
{
   return _db.get_chain_id();
}

dynamic_global_property_object database_api::get_dynamic_global_properties()const
{
   return my->get_dynamic_global_properties();
}

dynamic_global_property_object database_api_impl::get_dynamic_global_properties()const
{
   return _db.get(dynamic_global_property_id_type());
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<vector<account_id_type>> database_api::get_key_references( vector<public_key_type> key) const {
   return my->get_key_references( key );
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<vector<account_id_type>> database_api_impl::get_key_references( vector<public_key_type> keys) const
{
   wdump( (keys) );
   vector<vector<account_id_type>> final_result;
   final_result.reserve(keys.size());

   for (auto& key: keys)
   {

      address a1(pts_address(key, false, 56));
      address a2(pts_address(key, true, 56));
      address a3(pts_address(key, false, 0));
      address a4(pts_address(key, true, 0));
      address a5(key);

      subscribe_to_item(key);
      subscribe_to_item(a1);
      subscribe_to_item(a2);
      subscribe_to_item(a3);
      subscribe_to_item(a4);
      subscribe_to_item(a5);

      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
      auto itr = refs.account_to_key_memberships.find(key);
      vector<account_id_type> result;

      for (auto& a: {a1,a2,a3,a4,a5})
      {
         auto itr = refs.account_to_address_memberships.find(a);
         if (itr != refs.account_to_address_memberships.end())
         {
            result.reserve(itr->second.size());
            for (auto item: itr->second)
            {
               wdump((a)(item)(item(_db).name));
               result.push_back(item);
            }
         }
      }

      if (itr != refs.account_to_key_memberships.end())
      {
         result.reserve(itr->second.size());
         for (auto item: itr->second) {
            result.push_back(item);
         }
      }
      final_result.emplace_back(std::move(result));
   }

   for (auto i: final_result) {
      subscribe_to_item(i);
   }

   return final_result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Addresses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<vector<account_id_type>> database_api::get_address_references(vector<address> addresses) const {
   return my->get_address_references( addresses );
}

fc::optional<account_id_type> database_api::get_market_reference(const address& key) const {
   return my->get_market_reference(key);
}

address database_api::get_address(int64_t block_num, int64_t transaction_num) const {
   return address( block_num, transaction_num );
}

/**
 *  @return all accounts that referr to the address or account id in their owner or active authorities.
 */
vector<vector<account_id_type>> database_api_impl::get_address_references(vector<address> addresses) const
{
   wdump( (addresses) );
   vector<vector<account_id_type>> final_result;
   final_result.reserve(addresses.size());

   const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();

   for (auto& addr: addresses)
   {
      vector<account_id_type> result;

      auto a_itr = refs.account_to_address_memberships.find(addr);
      if (a_itr != refs.account_to_address_memberships.end())
      {
         result.reserve(a_itr->second.size());
         for (auto item: a_itr->second)
         {
            wdump((addr)(item)(item(_db).name));
            result.push_back(item);
         }
      }

      final_result.emplace_back(std::move(result));
   }

   for (auto i: final_result) {
      subscribe_to_item(i);
   }

   return final_result;
}

fc::optional<account_id_type> database_api_impl::get_market_reference(const address& key) const
{
   fc::optional<account_id_type> result;

   auto itr = _db.get_index_type<market_address_index>().indices().get<by_address>().find(key);
   FC_ASSERT(itr != _db.get_index_type<market_address_index>().indices().get<by_address>().end()
             , "There is no markets associated with this address: ${a}", ("a", key));

   result = itr->market_account_id;

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<account_object>> database_api::get_accounts(const vector<account_id_type>& account_ids) const {
   return my->get_accounts( account_ids );
}

std::pair<unsigned, vector<address>>
database_api::get_account_addresses(const string& name_or_id, unsigned from, unsigned limit) const {
   return my->get_account_addresses(name_or_id, from, limit);
}

vector<optional<account_object>> database_api_impl::get_accounts(const vector<account_id_type>& account_ids)const
{
   vector<optional<account_object>> result; result.reserve(account_ids.size());
   std::transform(account_ids.begin(), account_ids.end(), std::back_inserter(result),
                  [this](account_id_type id) -> optional<account_object> {
      if(auto o = _db.find(id))
      {
         subscribe_to_item( id );
         return *o;
      }
      return {};
   });
   return result;
}

std::pair<unsigned, vector<address>>
database_api_impl::get_account_addresses(const string& name_or_id, unsigned from, unsigned limit) const
{
   unsigned all_count = 0;
   vector<address> v_result;
   v_result.reserve(limit);

   optional<account_object> account_obj = get_account_by_name_or_id(name_or_id);
   FC_ASSERT( account_obj, "No such account with name_or_id '${n}'!", ("n", name_or_id) );

   all_count = account_obj->addresses.size();
   FC_ASSERT( (from < all_count), "Invalid argument 'from' (${v})", ("v", from) );

   auto itr = account_obj->addresses.begin() + from;

   while (itr != account_obj->addresses.end())
   {
      if (v_result.size() == limit) { break;}

      v_result.emplace_back(*itr);

      ++itr;
   }

   return {all_count, v_result};
}

std::map<string,full_account> database_api::get_full_accounts(const vector<string>& names_or_ids, bool subscribe) {
   return my->get_full_accounts(names_or_ids, subscribe);
}

std::map<std::string, full_account> database_api_impl::get_full_accounts(const vector<std::string>& names_or_ids, bool subscribe)
{
   idump((names_or_ids));
   std::map<std::string, full_account> results;

   for (const std::string& account_name_or_id : names_or_ids)
   {
      const account_object* account = nullptr;
      if (std::isdigit(account_name_or_id[0]))
         account = _db.find(fc::variant(account_name_or_id, 1).as<account_id_type>(1));
      else
      {
         const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
         auto itr = idx.find(account_name_or_id);
         if (itr != idx.end())
            account = &*itr;
      }
      if (account == nullptr) { continue; }

      if( subscribe )
      {
         ilog( "subscribe to ${id}", ("id",account->name) );
         subscribe_to_item( account->id );
      }

      // fc::mutable_variant_object full_account;
      full_account acnt;
      acnt.account = *account;
      acnt.statistics = account->statistics(_db);
      acnt.registrar_name = account->registrar(_db).name;
      acnt.referrer_name = account->referrer(_db).name;
      acnt.lifetime_referrer_name = account->lifetime_referrer(_db).name;
      acnt.votes = lookup_vote_ids( vector<vote_id_type>(account->options.votes.begin(),account->options.votes.end()) );

      // Add the account itself, its statistics object, cashback balance, and referral account names
      /*
      full_account("account", *account)("statistics", account->statistics(_db))
            ("registrar_name", account->registrar(_db).name)("referrer_name", account->referrer(_db).name)
            ("lifetime_referrer_name", account->lifetime_referrer(_db).name);
            */
      if (account->cashback_vb)
      {
         acnt.cashback_balance = account->cashback_balance(_db);
      }
      // Add the account's proposals
      const auto& proposal_idx = _db.get_index_type<proposal_index>();
      const auto& pidx = dynamic_cast<const primary_index<proposal_index>&>(proposal_idx);
      const auto& proposals_by_account = pidx.get_secondary_index<graphene::chain::required_approval_index>();
      auto  required_approvals_itr = proposals_by_account._account_to_proposals.find( account->id );
      if( required_approvals_itr != proposals_by_account._account_to_proposals.end() )
      {
         acnt.proposals.reserve( required_approvals_itr->second.size() );
         for( auto proposal_id : required_approvals_itr->second )
            acnt.proposals.push_back( proposal_id(_db) );
      }


      // Add the account's balances
      auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
      //vector<account_balance_object> balances;
      std::for_each(balance_range.first, balance_range.second,
                    [&acnt](const account_balance_object& balance) {
                       acnt.balances.emplace_back(balance);
                    });

      // Add the account's vesting balances
      auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(vesting_range.first, vesting_range.second,
                    [&acnt](const vesting_balance_object& balance) {
                       acnt.vesting_balances.emplace_back(balance);
                    });

      // Add the account's orders
      auto order_range = _db.get_index_type<limit_order_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(order_range.first, order_range.second,
                    [&acnt] (const limit_order_object& order) {
                       acnt.limit_orders.emplace_back(order);
                    });
      auto call_range = _db.get_index_type<call_order_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(call_range.first, call_range.second,
                    [&acnt] (const call_order_object& call) {
                       acnt.call_orders.emplace_back(call);
                    });
      const auto& index = _db.get_index_type<bonus_balances_index>().indices().get<by_account>();
      const auto& account_balances = index.find(account->id);
      if (account_balances != index.end())
         acnt.bonus_balances_id = account_balances->id;
      results[account_name_or_id] = acnt;
   }
   return results;
}

optional<bonus_balances_object> database_api::get_bonus_balances( string name_or_id ) const {
   return my->get_bonus_balances( name_or_id );
}
optional<bonus_balances_object> database_api_impl::get_bonus_balances( string name_or_id ) const
{
   optional<account_object> account_obj = get_account_by_name_or_id(name_or_id);
   if (!account_obj) {
      return optional<bonus_balances_object>();
   }
   optional<bonus_balances_object> obj;
   const auto& index = _db.get_index_type<bonus_balances_index>().indices().get<by_account>();
   const auto& account_balances = index.find(account_obj->id);
   return account_balances == index.end() ? optional<bonus_balances_object>() : *account_balances;
}

optional<account_object> database_api::get_account_by_name(string name) const {
   return my->get_account_by_name( name );
}

optional<account_object> database_api_impl::get_account_by_name( string name )const
{
   const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = idx.find(name);
   if (itr != idx.end())
      return *itr;
   return optional<account_object>();
}

optional<account_object> database_api::get_account_by_vote_id(const vote_id_type& v_id) const {
   return my->get_account_by_vote_id(v_id);
}

optional<account_object> database_api_impl::get_account_by_vote_id(const vote_id_type& v_id) const
{
   // witnesses
   if (v_id.type() == vote_id_type::vote_type::witness)
   {
      const auto& itr = _db.get_index_type<witness_index>().indices().get<by_vote_id>().find(v_id);
      if (itr != _db.get_index_type<witness_index>().indices().get<by_vote_id>().end())
      {
         const witness_object& obj = *itr;
         if (auto acc_itr = _db.find(obj.witness_account))
         {
            return *acc_itr;
         }
      }
   }
   // committee members
   else if (v_id.type() == vote_id_type::vote_type::committee)
   {
      const auto& itr = _db.get_index_type<committee_member_index>().indices().get<by_vote_id>().find(v_id);
      if (itr != _db.get_index_type<committee_member_index>().indices().get<by_vote_id>().end())
      {
         const committee_member_object& obj = *itr;
         if (auto acc_itr = _db.find(obj.committee_member_account)) {
            return *acc_itr;
         }
      }
   }

   return optional<account_object>();
}

optional<account_object> database_api_impl::get_account_by_name_or_id(const string& name_or_id) const
{
   optional<account_object> result;

   const account_object* account_ptr = nullptr;
   if (std::isdigit(name_or_id[0])) {
      account_ptr = _db.find(fc::variant(name_or_id, 1).as<account_id_type>(1));
   }
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name_or_id);
      if (itr != idx.end()) {
         account_ptr = &*itr;
      }
   }

   if (account_ptr) {
      result = *account_ptr;
   }

   return result;
}

vector<account_id_type> database_api::get_account_references( account_id_type account_id ) const {
   return my->get_account_references( account_id );
}

vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
{
   const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
   auto itr = refs.account_to_account_memberships.find(account_id);
   vector<account_id_type> result;

   if( itr != refs.account_to_account_memberships.end() )
   {
      result.reserve( itr->second.size() );
      for( auto item : itr->second ) result.push_back(item);
   }
   return result;
}

optional<restricted_account_object> database_api::get_restricted_account(account_id_type account_id) const {
   return my->get_restricted_account(account_id);
}

optional<restricted_account_object> database_api_impl::get_restricted_account(account_id_type account_id) const
{
   optional<restricted_account_object> result;

   auto itr = _db.get_index_type<restricted_account_index>().indices().get<by_acc_id>().find(account_id);
   if (itr != _db.get_index_type<restricted_account_index>().indices().get<by_acc_id>().end()) {
      result = *itr;
   }

   return result;
}

vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names) const {
   return my->lookup_account_names( account_names );
}

vector<optional<account_object>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   vector<optional<account_object> > result;
   result.reserve(account_names.size());
   std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                  [&accounts_by_name](const string& name) -> optional<account_object> {
      auto itr = accounts_by_name.find(name);
      return itr == accounts_by_name.end()? optional<account_object>() : *itr;
   });
   return result;
}

map<string,account_id_type> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_accounts( lower_bound_name, limit );
}

map<string,account_id_type> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   map<string,account_id_type> result;

   for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
        limit-- && itr != accounts_by_name.end();
        ++itr )
   {
      result.insert(make_pair(itr->name, itr->get_id()));
      if( limit == 1 )
         subscribe_to_item( itr->get_id() );
   }

   return result;
}

uint64_t database_api::get_account_count()const
{
   return my->get_account_count();
}

uint64_t database_api_impl::get_account_count()const
{
   return _db.get_index_type<account_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Balances                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<asset> database_api::get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const
{
   return my->get_account_balances( id, assets );
}

vector<asset> database_api_impl::get_account_balances(account_id_type acnt, const flat_set<asset_id_type>& assets)const
{
   vector<asset> result;
   if (assets.empty())
   {
      // if the caller passes in an empty list of assets, return balances for all assets the account owns
      const account_balance_index& balance_index = _db.get_index_type<account_balance_index>();
      auto range = balance_index.indices().get<by_account_asset>().equal_range(boost::make_tuple(acnt));
      for (const account_balance_object& balance : boost::make_iterator_range(range.first, range.second))
         result.push_back(asset(balance.get_balance()));
   }
   else
   {
      result.reserve(assets.size());

      std::transform(assets.begin(), assets.end(), std::back_inserter(result),
                     [this, acnt](asset_id_type id) { return _db.get_balance(acnt, id); });
   }

   return result;
}

vector<asset> database_api::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const
{
   return my->get_named_account_balances( name, assets );
}

vector<asset> database_api_impl::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets) const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = accounts_by_name.find(name);
   FC_ASSERT( itr != accounts_by_name.end() );
   return get_account_balances(itr->get_id(), assets);
}

vector<balance_object> database_api::get_balance_objects( const vector<address>& addrs )const
{
   return my->get_balance_objects( addrs );
}

vector<balance_object> database_api_impl::get_balance_objects( const vector<address>& addrs )const
{
   try
   {
      const auto& bal_idx = _db.get_index_type<balance_index>();
      const auto& by_owner_idx = bal_idx.indices().get<by_owner>();

      vector<balance_object> result;

      for( const auto& owner : addrs )
      {
         subscribe_to_item( owner );
         auto itr = by_owner_idx.lower_bound( boost::make_tuple( owner, asset_id_type(0) ) );
         while( itr != by_owner_idx.end() && itr->owner == owner )
         {
            result.push_back( *itr );
            ++itr;
         }
      }
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (addrs) )
}

vector<asset> database_api::get_vested_balances( const vector<balance_id_type>& objs )const
{
   return my->get_vested_balances( objs );
}

vector<asset> database_api_impl::get_vested_balances( const vector<balance_id_type>& objs )const
{
   try
   {
      vector<asset> result;
      result.reserve( objs.size() );
      auto now = _db.head_block_time();
      for( auto obj : objs )
         result.push_back( obj(_db).available( now ) );
      return result;
   } FC_CAPTURE_AND_RETHROW( (objs) )
}

vector<vesting_balance_object> database_api::get_vesting_balances( account_id_type account_id )const
{
   return my->get_vesting_balances( account_id );
}

vector<vesting_balance_object> database_api_impl::get_vesting_balances( account_id_type account_id )const
{
   try
   {
      vector<vesting_balance_object> result;
      auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account_id);
      std::for_each(vesting_range.first, vesting_range.second,
                    [&result](const vesting_balance_object& balance) {
                       result.emplace_back(balance);
                    });
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (account_id) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Assets                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<asset_object>> database_api::get_assets(const vector<asset_id_type>& asset_ids)const
{
   return my->get_assets( asset_ids );
}

vector<optional<asset_object>> database_api_impl::get_assets(const vector<asset_id_type>& asset_ids)const
{
   vector<optional<asset_object>> result; result.reserve(asset_ids.size());
   std::transform(asset_ids.begin(), asset_ids.end(), std::back_inserter(result),
                  [this](asset_id_type id) -> optional<asset_object> {
      if(auto o = _db.find(id))
      {
         subscribe_to_item( id );
         return *o;
      }
      return {};
   });
   return result;
}

vector<asset_object> database_api::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   return my->list_assets( lower_bound_symbol, limit );
}

vector<asset_object> database_api_impl::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   FC_ASSERT( limit <= 100 );
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<asset_object> result;
   result.reserve(limit);

   auto itr = assets_by_symbol.lower_bound(lower_bound_symbol);

   if( lower_bound_symbol == "" )
      itr = assets_by_symbol.begin();

   while(limit-- && itr != assets_by_symbol.end())
      result.emplace_back(*itr++);

   return result;
}

vector<optional<asset_object>> database_api::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   return my->lookup_asset_symbols( symbols_or_ids );
}

vector<optional<asset_object>> database_api_impl::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<optional<asset_object> > result;
   result.reserve(symbols_or_ids.size());
   std::transform(symbols_or_ids.begin(), symbols_or_ids.end(), std::back_inserter(result),
                  [this, &assets_by_symbol](const string& symbol_or_id) -> optional<asset_object> {
      if( !symbol_or_id.empty() && std::isdigit(symbol_or_id[0]) )
      {
         auto ptr = _db.find(variant(symbol_or_id, 1).as<asset_id_type>(1));
         return ptr == nullptr? optional<asset_object>() : *ptr;
      }
      auto itr = assets_by_symbol.find(symbol_or_id);
      return itr == assets_by_symbol.end()? optional<asset_object>() : *itr;
   });
   return result;
}

vector<fund_object> database_api::list_funds() const {
   return my->list_funds();
}

const fund_object* database_api_impl::get_fund_by_name_or_id(const std::string& fund_name_or_id) const
{
   FC_ASSERT(fund_name_or_id.size() > 0);
   const fund_object* fund_ptr = nullptr;

   if (std::isdigit(fund_name_or_id[0])) {
      fund_ptr = _db.find(fc::variant(fund_name_or_id, 1).as<fund_id_type>(1));
   }
   else
   {
      const auto& idx = _db.get_index_type<fund_index>().indices().get<by_name>();
      auto itr = idx.find(fund_name_or_id);
      if (itr != idx.end()) {
         fund_ptr = &*itr;
      }
   }
   FC_ASSERT( fund_ptr, "No such fund '${fund}'!", ("fund", fund_name_or_id) );

   return fund_ptr;
}

fund_object database_api::get_fund(const std::string& fund_name_or_id) const {
   return my->get_fund(fund_name_or_id);
}

const fund_object database_api_impl::get_fund(const std::string& fund_name_or_id) const
{
   const fund_object* fund_ptr = get_fund_by_name_or_id(fund_name_or_id);
   return *fund_ptr;
}

fund_object database_api::get_fund_by_owner(const std::string& account_name_or_id) const {
   return my->get_fund_by_owner(account_name_or_id);
}

fund_object database_api_impl::get_fund_by_owner(const std::string& account_name_or_id) const
{
   FC_ASSERT( account_name_or_id.size() > 0);
   const fund_object* fund_ptr = nullptr;

   const account_object* account = nullptr;
   if (std::isdigit(account_name_or_id[0])) {
      account = _db.find(fc::variant(account_name_or_id, 1).as<account_id_type>(1));
   }
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(account_name_or_id);
      if (itr != idx.end()) {
         account = &*itr;
      }
   }
   if (account)
   {
      const auto& idx = _db.get_index_type<fund_index>().indices().get<by_owner>();
      auto itr = idx.find(account->get_id());
      if (itr != idx.end()) {
         fund_ptr = &*itr;
      }
   }

   FC_ASSERT( fund_ptr, "There is no fund with owner '${owner}'!", ("owner", account_name_or_id) );

   return *fund_ptr;
}

////////////
// Funds  //
////////////

vector<fund_object> database_api_impl::list_funds() const
{
   const auto& funds_by_id = _db.get_index_type<fund_index>().indices().get<by_id>();
   vector<fund_object> result;
   result.reserve(100);

   auto itr = funds_by_id.begin();

   while (itr != funds_by_id.end()) {
      result.emplace_back(*itr++);
   }

   return result;
}

vector<fund_deposit_object> database_api::get_fund_deposits(const std::string& fund_name_or_id, uint32_t start, uint32_t limit) const {
   return my->get_fund_deposits(fund_name_or_id, start, limit);
}

vector<fund_deposit_object> database_api_impl::get_fund_deposits(const std::string& fund_name_or_id, uint32_t start, uint32_t limit) const
{
   FC_ASSERT( limit <= 100 );

   const fund_object* fund_ptr = get_fund_by_name_or_id(fund_name_or_id);

   vector<fund_deposit_object> result;
   result.reserve(limit);

   const auto& range = _db.get_index_type<fund_deposit_index>().indices().get<by_fund_id>().equal_range(fund_ptr->get_id());
   uint32_t i = 0;
   for (const fund_deposit_object& item: boost::make_iterator_range(range.first, range.second) | boost::adaptors::reversed)
   {
      if ( (i >= start) && (result.size() < limit) ) {
         result.emplace_back(item);
      }
      if (result.size() == limit) { break; }
      ++i;
   }

   return result;
}

pair<vector<fund_deposit_object>, uint32_t>
database_api::get_all_fund_deposits_by_period(uint32_t period, uint32_t start, uint32_t limit) const {
   return my->get_all_fund_deposits_by_period(period, start, limit);
}

pair<vector<fund_deposit_object>, uint32_t>
database_api_impl::get_all_fund_deposits_by_period(uint32_t period, uint32_t start, uint32_t limit) const
{
   vector<fund_deposit_object> result;
   result.reserve(100);
   uint32_t new_start = 0;

   const auto& dep_by_id = _db.get_index_type<fund_deposit_index>().indices().get<by_fund_id>();
   auto itr = dep_by_id.begin();
   uint32_t i = 0;
   while ( (i >= start) && (i < limit) && (itr != dep_by_id.end()) )
   {
      const fund_deposit_object& o = *itr;

      if (o.period == period)
      {
         result.emplace_back(o);
         new_start = i;
         ++i;
      }

      ++itr;
   }

   return std::make_pair(result, new_start);
}

asset database_api::get_fund_deposits_amount_by_account(fund_id_type fund_id, account_id_type account_id) const {
   return my->get_fund_deposits_amount_by_account(fund_id, account_id);
}

asset database_api_impl::get_fund_deposits_amount_by_account(fund_id_type fund_id, account_id_type account_id) const
{
   const fund_object* fund_ptr = _db.find(fc::variant(fund_id, 1).as<fund_id_type>(1));
   FC_ASSERT( fund_ptr, "No such fund '${fund}'!", ("fund", fund_id) );

   const auto& stats_obj = fund_id(_db).statistics_id(_db);
   auto iter = stats_obj.users_deposits.find(account_id);
   if (iter != stats_obj.users_deposits.end()) {
      return asset(iter->second, fund_ptr->get_asset_id());
   }

   return asset(0, fund_ptr->get_asset_id());
}

vector<fund_deposit_object> database_api::get_account_deposits(account_id_type account_id, uint32_t start, uint32_t limit) const {
   return my->get_account_deposits(account_id, start, limit);
}

vector<fund_deposit_object> database_api_impl::get_account_deposits(account_id_type account_id, uint32_t start, uint32_t limit) const
{
   vector<fund_deposit_object> result;

   const auto& range = _db.get_index_type<fund_deposit_index>().indices().get<by_account_id>().equal_range(account_id);
   uint32_t i = 0;
   for (const fund_deposit_object& item: boost::make_iterator_range(range.first, range.second) | boost::adaptors::reversed)
   {
      if ( (i >= start) && (result.size() < limit) ) {
         result.emplace_back(item);
      }
      if (result.size() == limit) { break; }
      ++i;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Markets / feeds                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<market_address_object> database_api::get_market_addresses(account_id_type account_id, uint32_t start, uint32_t limit) const {
   return my->get_market_addresses(account_id, start, limit);
}

vector<market_address_object> database_api_impl::get_market_addresses(account_id_type account_id, uint32_t start, uint32_t limit) const
{
   vector<market_address_object> result;

   const auto& range = _db.get_index_type<market_address_index>().indices().get<by_market_account_id>().equal_range(account_id);
   uint32_t i = 0;
   for (const market_address_object& item: boost::make_iterator_range(range.first, range.second) /*| boost::adaptors::reversed*/)
   {
      if ( (i >= start) && (result.size() < limit) ) {
         result.emplace_back(item);
      }
      if (result.size() == limit) { break; }
      ++i;
   }

   return result;
}

vector<limit_order_object> database_api::get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit) const {
   return my->get_limit_orders( a, b, limit );
}

/**
 *  @return the limit orders for both sides of the book for the two assets specified up to limit number on each side.
 */
vector<limit_order_object> database_api_impl::get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const
{
   const auto& limit_order_idx = _db.get_index_type<limit_order_index>();
   const auto& limit_price_idx = limit_order_idx.indices().get<by_price>();

   vector<limit_order_object> result;

   uint32_t count = 0;
   auto limit_itr = limit_price_idx.lower_bound(price::max(a,b));
   auto limit_end = limit_price_idx.upper_bound(price::min(a,b));
   while(limit_itr != limit_end && count < limit)
   {
      result.push_back(*limit_itr);
      ++limit_itr;
      ++count;
   }
   count = 0;
   limit_itr = limit_price_idx.lower_bound(price::max(b,a));
   limit_end = limit_price_idx.upper_bound(price::min(b,a));
   while(limit_itr != limit_end && count < limit)
   {
      result.push_back(*limit_itr);
      ++limit_itr;
      ++count;
   }

   return result;
}

vector<call_order_object> database_api::get_call_orders(asset_id_type a, uint32_t limit)const
{
   return my->get_call_orders( a, limit );
}

vector<call_order_object> database_api_impl::get_call_orders(asset_id_type a, uint32_t limit)const
{
   const auto& call_index = _db.get_index_type<call_order_index>().indices().get<by_price>();
   const asset_object& mia = _db.get(a);
   price index_price = price::min(mia.bitasset_data(_db).options.short_backing_asset, mia.get_id());

   return vector<call_order_object>(call_index.lower_bound(index_price.min()),
                                    call_index.lower_bound(index_price.max()));
}

map<account_id_type, uint16_t> database_api::get_online_info()const 
{
    return my->get_online_info();
}

map<account_id_type, uint16_t> database_api_impl::get_online_info()const
{
    return _db.get(accounts_online_id_type()).online_info;
}

vector<force_settlement_object> database_api::get_settle_orders(asset_id_type a, uint32_t limit)const
{
   return my->get_settle_orders( a, limit );
}

vector<force_settlement_object> database_api_impl::get_settle_orders(asset_id_type a, uint32_t limit)const
{
   const auto& settle_index = _db.get_index_type<force_settlement_index>().indices().get<by_expiration>();
   const asset_object& mia = _db.get(a);
   return vector<force_settlement_object>(settle_index.lower_bound(mia.get_id()),
                                          settle_index.upper_bound(mia.get_id()));
}

vector<call_order_object> database_api::get_margin_positions( const account_id_type& id )const
{
   return my->get_margin_positions( id );
}

vector<call_order_object> database_api_impl::get_margin_positions( const account_id_type& id )const
{
   try
   {
      const auto& idx = _db.get_index_type<call_order_index>();
      const auto& aidx = idx.indices().get<by_account>();
      auto start = aidx.lower_bound( boost::make_tuple( id, asset_id_type(0) ) );
      auto end = aidx.lower_bound( boost::make_tuple( id+1, asset_id_type(0) ) );
      vector<call_order_object> result;
      while( start != end )
      {
         result.push_back(*start);
         ++start;
      }
      return result;
   } FC_CAPTURE_AND_RETHROW( (id) )
}

void database_api::subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b)
{
   my->subscribe_to_market( callback, a, b );
}

void database_api_impl::subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b)
{
   if(a > b) std::swap(a,b);
   FC_ASSERT(a != b);
   _market_subscriptions[ std::make_pair(a,b) ] = callback;
}

void database_api::unsubscribe_from_market(asset_id_type a, asset_id_type b)
{
   my->unsubscribe_from_market( a, b );
}

void database_api_impl::unsubscribe_from_market(asset_id_type a, asset_id_type b)
{
   if(a > b) std::swap(a,b);
   FC_ASSERT(a != b);
   _market_subscriptions.erase(std::make_pair(a,b));
}

market_ticker database_api::get_ticker( const string& base, const string& quote )const
{
   return my->get_ticker( base, quote );
}

market_ticker database_api_impl::get_ticker( const string& base, const string& quote )const
{
   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;

   market_ticker result;

   result.base = base;
   result.quote = quote;
   result.base_volume = 0;
   result.quote_volume = 0;
   result.percent_change = 0;
   result.lowest_ask = 0;
   result.highest_bid = 0;

   //auto price_to_real = [&]( const share_type a, int p ) { return double( a.value ) / pow( 10, p ); };

   try {
      if( base_id > quote_id ) std::swap(base_id, quote_id);

      uint32_t day = 86400;
      auto now = fc::time_point_sec( fc::time_point::now() );
      auto orders = get_order_book( base, quote, 1 );
      auto trades = get_trade_history( base, quote, now, fc::time_point_sec( now.sec_since_epoch() - day ), 100 );

      result.latest = trades[0].price;

      for ( market_trade t: trades )
      {
         result.base_volume += t.value;
         result.quote_volume += t.amount;
      }

      while (trades.size() == 100)
      {
         trades = get_trade_history( base, quote, trades[99].date, fc::time_point_sec( now.sec_since_epoch() - day ), 100 );

         for ( market_trade t: trades )
         {
            result.base_volume += t.value;
            result.quote_volume += t.amount;
         }
      }

      trades = get_trade_history( base, quote, trades.back().date, fc::time_point_sec(), 1 );
      result.percent_change = trades.size() > 0 ? ( ( result.latest / trades.back().price ) - 1 ) * 100 : 0;

      //if (assets[0]->id == base_id)
      {
         result.lowest_ask = orders.asks[0].price;
         result.highest_bid = orders.bids[0].price;
      }

      return result;
   } FC_CAPTURE_AND_RETHROW( (base)(quote) )
}

market_volume database_api::get_24_volume( const string& base, const string& quote )const
{
   return my->get_24_volume( base, quote );
}

market_volume database_api_impl::get_24_volume( const string& base, const string& quote )const
{
   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;

   market_volume result;
   result.base = base;
   result.quote = quote;
   result.base_volume = 0;
   result.quote_volume = 0;

   try {
      if( base_id > quote_id ) std::swap(base_id, quote_id);

      uint32_t bucket_size = 86400;
      auto now = fc::time_point_sec( fc::time_point::now() );

      auto trades = get_trade_history( base, quote, now, fc::time_point_sec( now.sec_since_epoch() - bucket_size ), 100 );

      for ( market_trade t: trades )
      {
         result.base_volume += t.value;
         result.quote_volume += t.amount;
      }

      while (trades.size() == 100)
      {
         trades = get_trade_history( base, quote, trades[99].date, fc::time_point_sec( now.sec_since_epoch() - bucket_size ), 100 );

         for ( market_trade t: trades )
         {
            result.base_volume += t.value;
            result.quote_volume += t.amount;
         }
      }

      return result;
   } FC_CAPTURE_AND_RETHROW( (base)(quote) )
}

order_book database_api::get_order_book( const string& base, const string& quote, unsigned limit )const
{
   return my->get_order_book( base, quote, limit);
}

order_book database_api_impl::get_order_book( const string& base, const string& quote, unsigned limit )const
{
   using boost::multiprecision::uint128_t;
   FC_ASSERT( limit <= 50 );

   order_book result;
   result.base = base;
   result.quote = quote;

   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;
   auto orders = get_limit_orders( base_id, quote_id, limit );


   auto asset_to_real = [&]( const asset& a, int p ) { return double(a.amount.value)/pow( 10, p ); };
   auto price_to_real = [&]( const price& p )
   {
      if( p.base.asset_id == base_id )
         return asset_to_real( p.base, assets[0]->precision ) / asset_to_real( p.quote, assets[1]->precision );
      else
         return asset_to_real( p.quote, assets[0]->precision ) / asset_to_real( p.base, assets[1]->precision );
   };

   for( const auto& o : orders )
   {
      if( o.sell_price.base.asset_id == base_id )
      {
         order ord;
         ord.price = price_to_real( o.sell_price );
         ord.quote = asset_to_real( share_type( ( uint128_t( o.for_sale.value ) * o.sell_price.quote.amount.value ) / o.sell_price.base.amount.value ), assets[1]->precision );
         ord.base = asset_to_real( o.for_sale, assets[0]->precision );
         result.bids.push_back( ord );
      }
      else
      {
         order ord;
         ord.price = price_to_real( o.sell_price );
         ord.quote = asset_to_real( o.for_sale, assets[1]->precision );
         ord.base = asset_to_real( share_type( ( uint64_t( o.for_sale.value ) * o.sell_price.quote.amount.value ) / o.sell_price.base.amount.value ), assets[0]->precision );
         result.asks.push_back( ord );
      }
   }

   return result;
}

vector<market_trade> database_api::get_trade_history( const string& base,
                                                      const string& quote,
                                                      fc::time_point_sec start,
                                                      fc::time_point_sec stop,
                                                      unsigned limit )const
{
   return my->get_trade_history( base, quote, start, stop, limit );
}

vector<market_trade> database_api_impl::get_trade_history( const string& base,
                                                           const string& quote,
                                                           fc::time_point_sec start,
                                                           fc::time_point_sec stop,
                                                           unsigned limit )const
{
   FC_ASSERT( limit <= 100 );

   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;

   if( base_id > quote_id ) std::swap( base_id, quote_id );
   const auto& history_idx = _db.get_index_type<graphene::market_history::history_index>().indices().get<by_key>();
   history_key hkey;
   hkey.base = base_id;
   hkey.quote = quote_id;
   hkey.sequence = std::numeric_limits<int64_t>::min();

   auto price_to_real = [&]( const share_type a, int p ) { return double( a.value ) / pow( 10, p ); };

   if ( start.sec_since_epoch() == 0 )
      start = fc::time_point_sec( fc::time_point::now() );

   uint32_t count = 0;
   auto itr = history_idx.lower_bound( hkey );
   vector<market_trade> result;

   while( itr != history_idx.end() && count < limit && !( itr->key.base != base_id || itr->key.quote != quote_id || itr->time < stop ) )
   {
      if( itr->time < start )
      {
         market_trade trade;

         if( assets[0]->id == itr->op.receives.asset_id )
         {
            trade.amount = price_to_real( itr->op.pays.amount, assets[1]->precision );
            trade.value = price_to_real( itr->op.receives.amount, assets[0]->precision );
         }
         else
         {
            trade.amount = price_to_real( itr->op.receives.amount, assets[1]->precision );
            trade.value = price_to_real( itr->op.pays.amount, assets[0]->precision );
         }

         trade.date = itr->time;
         trade.price = trade.value / trade.amount;

         result.push_back( trade );
         ++count;
      }

      // Trades are tracked in each direction.
      ++itr;
      ++itr;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<witness_object>> database_api::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   return my->get_witnesses( witness_ids );
}

vector<worker_object> database_api::get_workers_by_account(account_id_type account)const
{
    const auto& idx = my->_db.get_index_type<worker_index>().indices().get<by_account>();
    auto itr = idx.find(account);
    vector<worker_object> result;

    if( itr != idx.end() && itr->worker_account == account )
    {
       result.emplace_back( *itr );
       ++itr;
    }

    return result;
}


vector<optional<witness_object>> database_api_impl::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   vector<optional<witness_object>> result; result.reserve(witness_ids.size());
   std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                  [this](witness_id_type id) -> optional<witness_object> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<witness_object> database_api::get_witness_by_account(account_id_type account)const
{
   return my->get_witness_by_account( account );
}

fc::optional<witness_object> database_api_impl::get_witness_by_account(account_id_type account) const
{
   const auto& idx = _db.get_index_type<witness_index>().indices().get<by_account>();
   auto itr = idx.find(account);
   if( itr != idx.end() )
      return *itr;
   return {};
}

map<string, witness_id_type> database_api::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_witness_accounts( lower_bound_name, limit );
}

map<string, witness_id_type> database_api_impl::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& witnesses_by_id = _db.get_index_type<witness_index>().indices().get<by_id>();

   // we want to order witnesses by account name, but that name is in the account object
   // so the witness_index doesn't have a quick way to access it.
   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of witnesses to be few and the frequency of calls to be rare
   std::map<std::string, witness_id_type> witnesses_by_account_name;
   for (const witness_object& witness : witnesses_by_id)
       if (auto account_iter = _db.find(witness.witness_account))
           if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
               witnesses_by_account_name.insert(std::make_pair(account_iter->name, witness.id));

   auto end_iter = witnesses_by_account_name.begin();
   while (end_iter != witnesses_by_account_name.end() && limit--)
       ++end_iter;
   witnesses_by_account_name.erase(end_iter, witnesses_by_account_name.end());
   return witnesses_by_account_name;
}

uint64_t database_api::get_witness_count()const
{
   return my->get_witness_count();
}

uint64_t database_api_impl::get_witness_count()const
{
   return _db.get_index_type<witness_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Committee members                                                //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<committee_member_object>> database_api::get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const
{
   return my->get_committee_members( committee_member_ids );
}

vector<optional<committee_member_object>> database_api_impl::get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const
{
   vector<optional<committee_member_object>> result; result.reserve(committee_member_ids.size());
   std::transform(committee_member_ids.begin(), committee_member_ids.end(), std::back_inserter(result),
                  [this](committee_member_id_type id) -> optional<committee_member_object> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<committee_member_object> database_api::get_committee_member_by_account(account_id_type account)const
{
   return my->get_committee_member_by_account( account );
}

fc::optional<committee_member_object> database_api_impl::get_committee_member_by_account(account_id_type account) const
{
   const auto& idx = _db.get_index_type<committee_member_index>().indices().get<by_account>();
   auto itr = idx.find(account);
   if( itr != idx.end() )
      return *itr;
   return {};
}

map<string, committee_member_id_type> database_api::lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_committee_member_accounts( lower_bound_name, limit );
}

map<string, committee_member_id_type> database_api_impl::lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& committee_members_by_id = _db.get_index_type<committee_member_index>().indices().get<by_id>();

   // we want to order committee_members by account name, but that name is in the account object
   // so the committee_member_index doesn't have a quick way to access it.
   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of committee_members to be few and the frequency of calls to be rare
   std::map<std::string, committee_member_id_type> committee_members_by_account_name;
   for (const committee_member_object& committee_member : committee_members_by_id)
       if (auto account_iter = _db.find(committee_member.committee_member_account))
           if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
               committee_members_by_account_name.insert(std::make_pair(account_iter->name, committee_member.id));

   auto end_iter = committee_members_by_account_name.begin();
   while (end_iter != committee_members_by_account_name.end() && limit--)
       ++end_iter;
   committee_members_by_account_name.erase(end_iter, committee_members_by_account_name.end());
   return committee_members_by_account_name;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Votes                                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<variant> database_api::lookup_vote_ids(const vector<vote_id_type>& votes) const {
   return my->lookup_vote_ids(votes);
}

vector<variant> database_api_impl::lookup_vote_ids(const vector<vote_id_type>& votes) const
{
   FC_ASSERT( votes.size() < 1000, "Only 1000 votes can be queried at a time" );

   const auto& witness_idx = _db.get_index_type<witness_index>().indices().get<by_vote_id>();
   const auto& committee_idx = _db.get_index_type<committee_member_index>().indices().get<by_vote_id>();
   const auto& for_worker_idx = _db.get_index_type<worker_index>().indices().get<by_vote_for>();
   const auto& against_worker_idx = _db.get_index_type<worker_index>().indices().get<by_vote_against>();

   vector<variant> result;
   result.reserve( votes.size() );
   for( auto id : votes )
   {
      switch( id.type() )
      {
         case vote_id_type::committee:
         {
            auto itr = committee_idx.find( id );
            if( itr != committee_idx.end() )
               result.emplace_back( variant( *itr, 1 ) );
            else
               result.emplace_back( variant() );
            break;
         }
         case vote_id_type::witness:
         {
            auto itr = witness_idx.find( id );
            if( itr != witness_idx.end() )
               result.emplace_back( variant( *itr, 1 ) );
            else
               result.emplace_back( variant() );
            break;
         }
         case vote_id_type::worker:
         {
            auto itr = for_worker_idx.find( id );
            if( itr != for_worker_idx.end() ) {
               result.emplace_back( variant( *itr, 1 ) );
            }
            else {
               auto itr = against_worker_idx.find( id );
               if( itr != against_worker_idx.end() ) {
                  result.emplace_back( variant( *itr, 1 ) );
               }
               else {
                  result.emplace_back( variant() );
               }
            }
            break;
         }
         case vote_id_type::VOTE_TYPE_COUNT: break; // supress unused enum value warnings
         default:
            FC_CAPTURE_AND_THROW( fc::out_of_range_exception, (id) );
      }
   }
   return result;
}

vector<account_id_type> database_api::get_voting_accounts(const vote_id_type& vote) const {
   return my->get_voting_accounts(vote);
}

vector<account_id_type> database_api_impl::get_voting_accounts(const vote_id_type& vote) const
{
   vector<account_id_type> result;

   const auto& idx = _db.get_index_type<account_index>().indices().get<by_id>();
   auto itr = idx.begin();
   while (itr != idx.end())
   {
      const account_object& acc = *itr;
      for (const vote_id_type& item: acc.options.votes)
      {
         if (item == vote)
         {
            result.push_back(acc.get_id());
            break;
         }
      }
      ++itr;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx)const
{
   return my->get_transaction_hex( trx );
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
{
   return fc::to_hex(fc::raw::pack(trx));
}

set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->get_required_signatures( trx, available_keys );
}

set<public_key_type> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   wdump((trx)(available_keys));
   auto result = trx.get_required_signatures( _db.get_chain_id(),
                                       available_keys,
                                       [this]( account_id_type id ){ return &id(_db).active; },
                                       [this]( account_id_type id ){ return &id(_db).owner; },
                                       _db.get_global_properties().parameters.max_authority_depth );
   wdump((result));
   return result;
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->get_potential_signatures( trx );
}
set<address> database_api::get_potential_address_signatures( const signed_transaction& trx )const
{
   return my->get_potential_address_signatures( trx );
}

set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
{
   wdump((trx));
   set<public_key_type> result;
   trx.get_required_signatures(
      _db.get_chain_id(),

      flat_set<public_key_type>(),

      [&]( account_id_type id )
      {
         const auto& auth = id(_db).active;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },

      [&]( account_id_type id )
      {
         const auto& auth = id(_db).owner;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },

      _db.get_global_properties().parameters.max_authority_depth
   );

   wdump((result));
   return result;
}

set<address> database_api_impl::get_potential_address_signatures( const signed_transaction& trx )const
{
   set<address> result;
   trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set<public_key_type>(),
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).active;
         for( const auto& k : auth.get_addresses() )
            result.insert(k);
         return &auth;
      },
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).owner;
         for( const auto& k : auth.get_addresses() )
            result.insert(k);
         return &auth;
      },
      _db.get_global_properties().parameters.max_authority_depth
   );
   return result;
}

bool database_api::verify_authority( const signed_transaction& trx )const
{
   return my->verify_authority( trx );
}

bool database_api_impl::verify_authority( const signed_transaction& trx )const
{
   trx.verify_authority( _db.get_chain_id(),
                         [&]( account_id_type id ){ return &id(_db).active; },
                         [&]( account_id_type id ){ return &id(_db).owner; },
                          _db.get_global_properties().parameters.max_authority_depth );
   return true;
}

bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
{
   return my->verify_account_authority( name_or_id, signers );
}

bool database_api_impl::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& keys )const
{
   FC_ASSERT( name_or_id.size() > 0);
   const account_object* account = nullptr;
   if (std::isdigit(name_or_id[0]))
      account = _db.find(fc::variant(name_or_id, 1).as<account_id_type>(1));
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name_or_id);
      if (itr != idx.end())
         account = &*itr;
   }
   FC_ASSERT( account, "no such account" );

   /// reuse trx.verify_authority by creating a dummy transfer
   signed_transaction trx;
   transfer_operation op;
   op.from = account->id;
   trx.operations.emplace_back(op);

   return verify_authority( trx );
}

processed_transaction database_api::validate_transaction(const signed_transaction& trx) const {
   return my->validate_transaction(trx);
}

processed_transaction database_api_impl::validate_transaction(const signed_transaction& trx) const {
   return _db.validate_transaction(trx);
}

vector<fc::variant> database_api::get_required_fees(const vector<operation>& ops, asset_id_type id) const {
   return my->get_required_fees( ops, id );
}

/**
 * Container method for mutually recursive functions used to
 * implement get_required_fees() with potentially nested proposals.
 */
struct get_required_fees_helper
{
   get_required_fees_helper(
      const fee_schedule& _current_fee_schedule
      , const price& _core_exchange_rate
      , uint32_t _max_recursion):
         current_fee_schedule(_current_fee_schedule)
         , core_exchange_rate(_core_exchange_rate)
         , max_recursion(_max_recursion) { }

   fc::variant set_op_fees( operation& op )
   {
      if (op.which() == operation::tag<proposal_create_operation>::value) {
         return set_proposal_create_op_fees( op );
      }
      else
      {
         asset fee = current_fee_schedule.set_fee( op, core_exchange_rate );
         fc::variant result;
         fc::to_variant( fee, result, GRAPHENE_NET_MAX_NESTED_OBJECTS );
         return result;
      }
   }

   fc::variant set_proposal_create_op_fees( operation& proposal_create_op )
   {
      proposal_create_operation& op = proposal_create_op.get<proposal_create_operation>();
      std::pair< asset, fc::variants > result;
      for( op_wrapper& prop_op : op.proposed_ops )
      {
         FC_ASSERT( current_recursion < max_recursion );
         ++current_recursion;
         result.second.push_back( set_op_fees( prop_op.op ) );
         --current_recursion;
      }
      // we need to do this on the boxed version, which is why we use
      // two mutually recursive functions instead of a visitor
      result.first = current_fee_schedule.set_fee( proposal_create_op, core_exchange_rate );
      fc::variant vresult;
      fc::to_variant( result, vresult, GRAPHENE_NET_MAX_NESTED_OBJECTS );
      return vresult;
   }

   const fee_schedule& current_fee_schedule;
   const price& core_exchange_rate;
   uint32_t max_recursion;
   uint32_t current_recursion = 0;
};

vector< fc::variant > database_api_impl::get_required_fees( const vector<operation>& ops, asset_id_type id )const
{
   vector< operation > _ops = ops;
   //
   // we copy the ops because we need to mutate an operation to reliably
   // determine its fee, see #435
   //

   vector< fc::variant > result;
   result.reserve(ops.size());
   const asset_object& a = id(_db);
   get_required_fees_helper helper(
      _db.current_fee_schedule(),
      a.options.core_exchange_rate,
      GET_REQUIRED_FEES_MAX_RECURSION );
   for( operation& op : _ops )
   {
      result.push_back( helper.set_op_fees( op ) );
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Proposed transactions                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<proposal_object> database_api::get_proposed_transactions(account_id_type id) const {
   return my->get_proposed_transactions(id);
}

/** TODO: add secondary index that will accelerate this process */
vector<proposal_object> database_api_impl::get_proposed_transactions( account_id_type id )const
{
   const auto& idx = _db.get_index_type<proposal_index>();
   vector<proposal_object> result;

   idx.inspect_all_objects( [&](const object& obj){
           const proposal_object& p = static_cast<const proposal_object&>(obj);
           if( p.required_active_approvals.find( id ) != p.required_active_approvals.end() )
              result.push_back(p);
           else if ( p.required_owner_approvals.find( id ) != p.required_owner_approvals.end() )
              result.push_back(p);
           else if ( p.available_active_approvals.find( id ) != p.available_active_approvals.end() )
              result.push_back(p);
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blinded balances                                                 //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<blinded_balance_object> database_api::get_blinded_balances(const flat_set<commitment_type>& commitments) const {
   return my->get_blinded_balances( commitments );
}

vector<blinded_balance_object> database_api_impl::get_blinded_balances( const flat_set<commitment_type>& commitments )const
{
   vector<blinded_balance_object> result; result.reserve(commitments.size());
   const auto& bal_idx = _db.get_index_type<blinded_balance_index>();
   const auto& by_commitment_idx = bal_idx.indices().get<by_commitment>();
   for( const auto& c : commitments )
   {
      auto itr = by_commitment_idx.find( c );
      if( itr != by_commitment_idx.end() )
         result.push_back( *itr );
   }
   return result;
}

optional<cheque_info_object> database_api::get_cheque_by_code(const std::string& code) const {
   return my->get_cheque_by_code(code);
}

optional<cheque_info_object> database_api_impl::get_cheque_by_code(const std::string& code) const
{
   FC_ASSERT(code.length() == 16, "invalid cheque code!");

   optional<cheque_info_object> result;

   auto idx = _db.get_index_type<cheque_index>().indices().get<by_code>().find(code);
   FC_ASSERT(idx != _db.get_index_type<cheque_index>().indices().get<by_code>().end(), "There is no cheque with this code!");

   const cheque_object& ch_obj = *idx;

   cheque_info_object obj;
   obj.id                  = ch_obj.get_id();
   obj.datetime_expiration = ch_obj.datetime_expiration;
   obj.payee_amount        = asset(ch_obj.amount_payee, ch_obj.asset_id);

   return obj;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Private methods                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api_impl::broadcast_updates( const vector<variant>& updates )
{
   if( updates.size() ) {
      auto capture_this = shared_from_this();
      fc::async([capture_this,updates](){
          capture_this->_subscribe_callback( fc::variant(updates) );
      });
   }
}

void database_api_impl::on_objects_removed( const vector<const object*>& objs )
{
   /// we need to ensure the database_api is not deleted for the life of the async operation
   if( _subscribe_callback )
   {
      vector<variant>    updates;
      updates.reserve(objs.size());

      for( auto obj : objs )
         updates.emplace_back( obj->to_variant() );
      broadcast_updates( updates );
   }

   if( _market_subscriptions.size() )
   {
      map< pair<asset_id_type, asset_id_type>, vector<variant> > broadcast_queue;
      for( const auto& obj : objs )
      {
         const limit_order_object* order = dynamic_cast<const limit_order_object*>(obj);
         if( order )
         {
            auto sub = _market_subscriptions.find( order->get_market() );
            if( sub != _market_subscriptions.end() )
               broadcast_queue[order->get_market()].emplace_back( order->to_variant() );
         }
      }
      if( broadcast_queue.size() )
      {
         auto capture_this = shared_from_this();
         fc::async([capture_this,this,broadcast_queue](){
             for( const auto& item : broadcast_queue )
             {
               auto sub = _market_subscriptions.find(item.first);
               if( sub != _market_subscriptions.end() )
                   sub->second( fc::variant(item.second ) );
             }
         });
      }
   }
}

void database_api_impl::on_objects_changed(const vector<object_id_type>& ids)
{
   if (_db.start_notify_block_num >= _db.head_block_num()) return;
   vector<variant>    updates;
   map< pair<asset_id_type, asset_id_type>,  vector<variant> > market_broadcast_queue;
   for(auto id : ids)
   {
      if (id == ALPHA_ACCOUNT_ID) continue;

      const vector<optional<asset_object>>& assets = get_assets({EDC_ASSET});
      if ( (assets.size() > 0) && assets[0].valid() && (assets[0]->issuer == id) ) { continue; }

      const object* obj = nullptr;
      if( _subscribe_callback )
      {
         obj = _db.find_object( id );
         if( obj )
         {
            updates.emplace_back( obj->to_variant() );
         }
         else
         {
            updates.emplace_back(fc::variant( id, 1 )); // send just the id to indicate removal
         }
      }

      if( _market_subscriptions.size() )
      {
         if( !_subscribe_callback )
            obj = _db.find_object( id );
         if( obj )
         {
            const limit_order_object* order = dynamic_cast<const limit_order_object*>(obj);
            if( order )
            {
               auto sub = _market_subscriptions.find( order->get_market() );
               if( sub != _market_subscriptions.end() )
                  market_broadcast_queue[order->get_market()].emplace_back( order->to_variant() );
            }
         }
      }
   }

   auto capture_this = shared_from_this();

   /// pushing the future back / popping the prior future if it is complete.
   /// if a connection hangs then this could get backed up and result in
   /// a failure to exit cleanly.
   fc::async([capture_this,this,updates,market_broadcast_queue](){
      if( _subscribe_callback ) _subscribe_callback( updates );

      for( const auto& item : market_broadcast_queue )
      {
        auto sub = _market_subscriptions.find(item.first);
        if( sub != _market_subscriptions.end() )
            sub->second( fc::variant(item.second ) );
      }
   });
}

/** note: this method cannot yield because it is called in the middle of
 * apply a block.
 */
void database_api_impl::on_applied_block()
{
   if (_block_applied_callback)
   {
      auto capture_this = shared_from_this();
      block_id_type block_id = _db.head_block_id();
      fc::async([this,capture_this,block_id](){
         _block_applied_callback(fc::variant(block_id, 1));
      });
   }

   if(_market_subscriptions.size() == 0)
      return;

   const auto& ops = _db.get_applied_operations();
   map< std::pair<asset_id_type,asset_id_type>, vector<pair<operation, operation_result>> > subscribed_markets_ops;
   for(const optional< operation_history_object >& o_op : ops)
   {
      if( !o_op.valid() )
         continue;
      const operation_history_object& op = *o_op;

      std::pair<asset_id_type,asset_id_type> market;
      switch(op.op.which())
      {
         /*  This is sent via the object_changed callback
         case operation::tag<limit_order_create_operation>::value:
            market = op.op.get<limit_order_create_operation>().get_market();
            break;
         */
         case operation::tag<fill_order_operation>::value:
            market = op.op.get<fill_order_operation>().get_market();
            break;
            /*
         case operation::tag<limit_order_cancel_operation>::value:
         */
         default: break;
      }
      if(_market_subscriptions.count(market))
         subscribed_markets_ops[market].push_back(std::make_pair(op.op, op.result));
   }
   /// we need to ensure the database_api is not deleted for the life of the async operation
   auto capture_this = shared_from_this();
   fc::async([this,capture_this,subscribed_markets_ops](){
      for(auto item : subscribed_markets_ops)
      {
         auto itr = _market_subscriptions.find(item.first);
         if(itr != _market_subscriptions.end())
            itr->second(fc::variant(item.second, GRAPHENE_NET_MAX_NESTED_OBJECTS));
      }
   });
}    
ref_info database_api::get_referrals_by_id(string account_name_or_id) {
   auto account = get_account_by_name(account_name_or_id);
   FC_ASSERT(account.valid(), "invalid account");
   return my->get_referrals_by_id(account);
}
ref_info database_api_impl::get_referrals_by_id( optional<account_object> account ) const {
    const auto& idx = _db.get_index_type<chain::account_index>();
    auto asset = _db.get_index_type<asset_index>().indices().get<by_symbol>().find(EDC_ASSET_SYMBOL);
    auto& bal_idx = _db.get_index_type<account_balance_index>();
    referral_tree rtree( idx, bal_idx, asset->id, account->id );
    rtree.form_old();
    leaf_info root = *rtree.referral_map.find(account->id)->second;
    ref_info result( root, account->name );
    for (child_balance e: root.child_balances) {
        if (e.level == 1) {
            result.level_1.push_back(ref_info(*rtree.referral_map.find(e.account_id)->second, e.account_id(_db).name));
        }
    }
    return result;
}

Unit database_api::get_referrals(string account_name_or_id) {
   auto account = get_account_by_name(account_name_or_id);
   FC_ASSERT(account.valid(), "invalid account");
   return my->get_referrals(account);
}
Unit database_api_impl::get_referrals( optional<account_object> account ) const {
    const auto& idx = _db.get_index_type<chain::account_index>();
    auto asset = _db.get_index_type<asset_index>().indices().get<by_symbol>().find(EDC_ASSET_SYMBOL);
    Unit start(account->get_id(), account->name, _db.get_balance(account->id, asset->id).amount.value);
    std::map<account_id_type, Unit*> search;
    search.emplace(account->get_id(), &start);
    idx.inspect_all_objects( [&](const chain::object& obj){
        const account_object& ref = static_cast<const chain::account_object&>(obj);
        auto searchIt = search.find(ref.referrer);
        if (searchIt != search.end()) {
            auto balance = _db.get_balance(ref.id, asset->id).amount.value;
            auto nElem = Unit(ref.get_id(), ref.name, balance);
//            auto smth = search.emplace(ref.get_id(), nElem);
            searchIt->second->referrals.push_back(nElem);
        }
    });  
    return start;
}

vector<SimpleUnit> database_api::get_accounts_info(vector<string> account_names_or_ids)
{
   vector<optional<account_object>> accs;
   accs.reserve(account_names_or_ids.size());
   for( string acc_name_or_id : account_names_or_ids )
   {
      const account_object* account_ptr = nullptr;
      if (std::isdigit(acc_name_or_id[0])) {
         account_ptr = my->_db.find(fc::variant(acc_name_or_id, 1).as<account_id_type>(1));
      }
      else
      {
         const auto& idx = my->_db.get_index_type<account_index>().indices().get<by_name>();
         auto itr = idx.find(acc_name_or_id);
         if (itr != idx.end()) {
            account_ptr = &*itr;
         }
      }

      if (account_ptr) {
         accs.push_back(*account_ptr);
      }
   }
   return my->get_accounts_info(accs);
}

vector<SimpleUnit> database_api_impl::get_accounts_info(vector<optional<account_object>> accounts)
{
   vector<SimpleUnit> ret(accounts.size());
   list<referral_tree> referral_set;

   struct acc_index{
      int n;
      optional<account_object>* acc;
      acc_index(int n, optional<account_object>* acc) : n(n), acc(acc){};
   };

   vector<acc_index> sorted_accounts;

   for (size_t i = 0; i < accounts.size(); i++)
   {
      if(accounts[i].valid())
         sorted_accounts.push_back(acc_index(i, &accounts[i]));
   }

   sort(sorted_accounts.begin(), sorted_accounts.end(), [](const acc_index & a, const acc_index & b) -> bool
         {
            if (a.acc->valid() && b.acc->valid())
            {
               return (**(a.acc)).id < (**(b.acc)).id;
            }
            return a.acc->valid() < b.acc->valid();
         });

   for (acc_index idx : sorted_accounts)
   {
      if (!idx.acc)
      {
         continue;
      }
      account_object acc_obj = **(idx.acc);
      SimpleUnit ret_unit;
      bool found = false;
      for (referral_tree ref_tree : referral_set)
      {
         auto it = ref_tree.referral_map.find(acc_obj.id);
         if (it != ref_tree.referral_map.end())
         {
            leaf_info info = *it->second;

            ret_unit.balance = info.balance;
            ret_unit.id = info.account_id;
            ret_unit.name = acc_obj.name;
            ret_unit.rank = info.rank;
            ret[idx.n] = ret_unit;
            found = true;
            break;
         }
      }
      if (found) continue;
      const auto& db_idx = _db.get_index_type<chain::account_index>();
      auto asset = _db.get_index_type<asset_index>().indices().get<by_symbol>().find(EDC_ASSET_SYMBOL);
      auto& bal_idx = _db.get_index_type<account_balance_index>();
      referral_set.push_back(referral_tree( db_idx, bal_idx, asset->id, acc_obj.get_id() ));
      referral_set.back().form_old();
      referral_set.back().scan_old();
      ret_unit.balance =      referral_set.back().root.node->data.balance;
      ret_unit.id =           referral_set.back().root.node->data.account_id;
      ret_unit.name =         acc_obj.name;
      ret_unit.rank =         referral_set.back().root.node->data.rank;
      ret[idx.n] = ret_unit;

   }

   sorted_accounts.clear();
   return ret;
}

fc::variant_object database_api::get_user_count_by_ranks() 
{
   return my->get_user_count_by_ranks();
}

fc::variant_object database_api_impl::get_user_count_by_ranks() const
{
   std::map<string, uint64_t> mapres;    
   mapres.emplace("A", 0);
   mapres.emplace("B", 0);
   mapres.emplace("C", 0);
   mapres.emplace("D", 0);
   mapres.emplace("E", 0);
   mapres.emplace("F", 0);
   mapres.emplace("G", 0);
   const auto& idx = _db.get_index_type<chain::account_index>();
   auto asset = _db.get_index_type<asset_index>().indices().get<by_symbol>().find(EDC_ASSET_SYMBOL);
   auto& bal_idx = _db.get_index_type<account_balance_index>();
   referral_tree rtree( idx, bal_idx, asset->id );
   rtree.form_old();
   auto refbonuses = rtree.scan_old();
   for (auto& elem: rtree.tree_data) {
      if (!elem.rank.empty())
         mapres[elem.rank]++;
   }
   fc::mutable_variant_object result;
   for (auto& e: mapres) {
      result[e.first] = e.second;
   }
   return result;
}
// get_user_count_with_balances ["1970-01-01T00:00:00", "1970-01-01T00:00:00"]
int64_t database_api::get_user_count_with_balances(std::vector<fc::time_point_sec> dates) 
{
   fc::time_point_sec start, end;
   if (dates.size() == 1) {
      start = dates[0];
   } else if (dates.size() == 2) {
      start = dates[0];
      end = dates[1];
      if (start > end) std::swap(start, end);
   }
   return my->get_user_count_with_balances(start, end);
}

int64_t database_api_impl::get_user_count_with_balances(fc::time_point_sec start, fc::time_point_sec end) const 
{
   const auto& idx = _db.get_index_type<chain::account_index>().indices().get<by_id>();
   auto asset = _db.get_index_type<asset_index>().indices().get<by_symbol>().find(EDC_ASSET_SYMBOL);
   int64_t users_count = 0;
   if (start == fc::time_point_sec() && end == fc::time_point_sec()) {
      for (auto account = ++idx.begin(); account != idx.end(); account++) {
         auto balance = _db.get_balance(account->id, asset->id).amount.value;
         if (balance < 1) continue;
         users_count++;
      }
   } else {
      if (end == fc::time_point_sec())
         end = fc::time_point::now();
      for (auto account = ++idx.begin(); account != idx.end(); account++) {
         auto balance = _db.get_balance(account->id, asset->id).amount.value;
         if (balance < 1) continue;

         const auto& stats = account->statistics(_db);
         const account_transaction_history_object* node = &stats.most_recent_op(_db);
         //uint32_t ops_count = stats.total_ops;
         while (true) {
            if (node->next == account_transaction_history_id_type()) break;
            node = &node->next(_db);
         }
         auto& hist = node->operation_id(_db);
         if (hist.op.which() == 5) { // account_create_operation 
            auto op = hist.op.get<account_create_operation>();
            fc::time_point_sec create_time = _db.fetch_block_by_number(hist.block_num)->timestamp;
            if (create_time >= start && create_time <= end) {
               users_count++;
            }
         }
      }
   }
   return users_count;
}

std::pair<uint32_t, std::vector<account_id_type>>
database_api::get_users_with_asset(const asset_id_type& asst, uint32_t start, uint32_t limit) const {
   return my->get_users_with_asset(asst, start, limit);
}

std::pair<uint32_t, std::vector<account_id_type>>
database_api_impl::get_users_with_asset(const asset_id_type& asst, uint32_t start, uint32_t limit) const
{
   FC_ASSERT(limit <= 100);

   const vector<optional<asset_object>>& assets = get_assets({asst});
   FC_ASSERT( ((assets.size() > 0) && assets[0].valid()), "There is no asset with ID ${id}", ("id", asst));

   uint32_t last_item_num = 0;
   vector<account_id_type> v_result;
   v_result.reserve(limit);

   const auto& idx = _db.get_index_type<account_balance_index>().indices().get<by_id>();
   uint32_t i = 0;
   auto itr = idx.begin();

   while (itr != idx.end())
   {
      const account_balance_object& obj = *itr;

      if ( (i >= start) && (v_result.size() < limit) && (obj.asset_type == asst) && (obj.balance > 0) ) {
         v_result.emplace_back(obj.owner);
      }
      if (v_result.size() == limit)
      {
         last_item_num = i;
         break;
      }

      ++i;
      ++itr;
   }

   return {last_item_num, v_result};
}

} } // graphene::app
