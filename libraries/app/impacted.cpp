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

#include <graphene/protocol/authority.hpp>
#include <graphene/app/impacted.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/fund_object.hpp>

namespace graphene { namespace app {

using namespace fc;
using namespace graphene::chain;

// TODO:  Review all of these, especially no-ops
struct get_impacted_items_visitor
{
   flat_set<account_id_type>& _impacted_accounts;
   flat_set<fund_id_type>& _impacted_funds;
   graphene::chain::database* db_ptr = nullptr;

   get_impacted_items_visitor( flat_set<account_id_type>& impact_acc, flat_set<fund_id_type>& impact_fund, graphene::chain::database* db_ptr):
    _impacted_accounts(impact_acc)
    , _impacted_funds(impact_fund)
    , db_ptr(db_ptr) { }
   typedef void result_type;

   void operator()( const transfer_operation& op ) {
      _impacted_accounts.insert( op.to );
   }
   void operator()( const update_blind_transfer2_settings_operation& op ) { }
   void operator()( const blind_transfer2_operation& op ) {
      _impacted_accounts.insert(op.to);
   }
   void operator()( const update_settings_operation& op ) { }
   void operator()( const deposit_renewal_operation& op ) {
      _impacted_accounts.insert( op.account_id );
   }
   void operator()( const set_market_operation& op ) {
      _impacted_accounts.insert( op.to_account );
   }

   void operator()( const asset_claim_fees_operation& op ) { }
   void operator()( const limit_order_create_operation& op ) { }
   void operator()( const limit_order_cancel_operation& op ) {
      _impacted_accounts.insert( op.fee_paying_account );
   }
   void operator()( const call_order_update_operation& op ) { }
   void operator()( const fill_order_operation& op ) {
      _impacted_accounts.insert( op.account_id );
   }

   void operator()( const account_create_operation& op )
   {
      _impacted_accounts.insert( op.registrar );
      _impacted_accounts.insert( op.referrer );
      add_authority_accounts( _impacted_accounts, op.owner );
      add_authority_accounts( _impacted_accounts, op.active );
   }

   void operator()(const account_update_operation& op)
   {
      _impacted_accounts.insert(op.account);

      if (op.owner) {
         add_authority_accounts(_impacted_accounts, *(op.owner));
      }
      if (op.active) {
         add_authority_accounts(_impacted_accounts, *(op.active));
      }
   }
   void operator()(const account_update_authorities_operation& op)
   {
      _impacted_accounts.insert(op.account);

      if (op.owner) {
         add_authority_accounts(_impacted_accounts, *(op.owner));
      }
      if (op.active) {
         add_authority_accounts(_impacted_accounts, *(op.active));
      }
   }

   void operator()( const add_address_operation& op ) {}

   void operator()( const account_whitelist_operation& op ) {
      _impacted_accounts.insert( op.account_to_list );
   }

   void operator()( const account_restrict_operation& op ) {
      _impacted_accounts.insert( op.target );
   }

   void operator()( const account_allow_registrar_operation& op ) {
      _impacted_accounts.insert( op.target );
   }

   void operator()( const set_online_time_operation& op ) { }

   void operator()( const set_verification_is_required_operation& op ) {
      _impacted_accounts.insert( op.target );
   }

   void operator()( const allow_create_addresses_operation& op ) { }

   void operator()( const set_burning_mode_operation& op ) {
      _impacted_accounts.insert( op.account_id );
   }

   void operator()( const assets_update_fee_payer_operation& op ) { }
   void operator()( const asset_update_exchange_rate_operation& op ) { }

   void operator()( const account_upgrade_operation& op ) { }
   void operator()( const account_transfer_operation& op ) {
      _impacted_accounts.insert( op.new_owner );
   }

   void operator()( const asset_create_operation& op ) { }
   void operator()( const allow_create_asset_operation& op) { }

   void operator()( const fund_create_operation& op) {
      _impacted_accounts.insert(op.owner);
   }
   void operator()( const fund_update_operation& op)
   {
      _impacted_accounts.insert(op.from_account);
      _impacted_funds.insert(op.id);
   }
   void operator()( const fund_refill_operation& op)
   {
      _impacted_accounts.insert(op.from_account);
      _impacted_funds.insert(op.id);
   }
   void operator()( const fund_deposit_operation& op)
   {
      _impacted_accounts.insert(op.from_account);
      _impacted_funds.insert(op.fund_id);
   }
   void operator()( const fund_withdrawal_operation& op)
   {
      _impacted_accounts.insert(op.issue_to_account);
      _impacted_funds.insert(op.fund_id);
   }
   void operator()( const fund_payment_operation& op)
   {
      _impacted_accounts.insert(op.issue_to_account);
      _impacted_funds.insert(op.fund_id);
   }
   void operator()( const fund_set_enable_operation& op) {
      _impacted_funds.insert(op.id);
   }
   void operator()( const fund_deposit_set_enable_operation& op)
   {
      if (db_ptr)
      {
         auto dep_itr = db_ptr->find(op.deposit_id);
         if (dep_itr)
         {
            auto dep_owner_itr = db_ptr->find(dep_itr->account_id);
            if (dep_owner_itr) {
               _impacted_accounts.insert(dep_owner_itr->get_id());
            }
         }
      }
   }

   void operator()( const fund_remove_operation& op) {
      _impacted_accounts.insert(ALPHA_ACCOUNT_ID);
   }
   void operator()( const fund_change_payment_scheme_operation& op)
   {
      _impacted_accounts.insert(ALPHA_ACCOUNT_ID);
      _impacted_funds.insert(op.id);
   }
   void operator()( const enable_autorenewal_deposits_operation& op) {
      _impacted_accounts.insert(op.account_id);
   }
   void operator()( const asset_update_operation& op )
   {
      if ( op.new_issuer ) {
         _impacted_accounts.insert(*(op.new_issuer));
      }
   }
   void operator()( const asset_update2_operation& op )
   {
      if ( op.new_issuer ) {
         _impacted_accounts.insert(*(op.new_issuer));
      }
   }

   void operator()( const asset_update_bitasset_operation& op ) {}
   void operator()( const asset_update_feed_producers_operation& op ) {}

   void operator()( const asset_issue_operation& op ) {
      _impacted_accounts.insert( op.issue_to_account );
   }
   void operator()( const bonus_operation& op ) { }

   void operator()( const referral_issue_operation& op ) {

      _impacted_accounts.insert( op.issue_to_account );
   }
   void operator()( const daily_issue_operation& op ) {
      _impacted_accounts.insert( op.issue_to_account );
   }

   void operator()( const asset_reserve_operation& op ) {}
   void operator()( const asset_fund_fee_pool_operation& op ) {}
   void operator()( const edc_asset_fund_fee_pool_operation& op ) {}
   void operator()( const asset_settle_operation& op ) {}
   void operator()( const asset_global_settle_operation& op ) {}
   void operator()( const asset_publish_feed_operation& op ) {}
   void operator()( const witness_create_operation& op ) {
      _impacted_accounts.insert( op.witness_account );
   }
   void operator()( const witness_update_operation& op ) {
      _impacted_accounts.insert( op.witness_account );
   }

   void operator()( const proposal_create_operation& op )
   {
      vector<authority> other;
      for (const auto& proposed_op : op.proposed_ops) {
         operation_get_required_authorities( proposed_op.op, _impacted_accounts, _impacted_accounts, other );
      }
      for (auto& o : other) {
         add_authority_accounts(_impacted_accounts, o);
      }
   }

   void operator()( const proposal_update_operation& op ) { }
   void operator()( const proposal_delete_operation& op ) { }

   void operator()( const withdraw_permission_create_operation& op ) {
      _impacted_accounts.insert( op.authorized_account );
   }

   void operator()( const withdraw_permission_update_operation& op ) {
      _impacted_accounts.insert( op.authorized_account );
   }

   void operator()( const withdraw_permission_claim_operation& op ) {
      _impacted_accounts.insert( op.withdraw_from_account );
   }

   void operator()( const withdraw_permission_delete_operation& op ) {
      _impacted_accounts.insert( op.authorized_account );
   }

   void operator()( const committee_member_create_operation& op ) {
      _impacted_accounts.insert( op.committee_member_account );
   }
   void operator()( const committee_member_update_operation& op ) {
      _impacted_accounts.insert( op.committee_member_account );
   }
   void operator()( const committee_member_update_global_parameters_operation& op ) { }

   void operator()( const vesting_balance_create_operation& op ) {
      _impacted_accounts.insert( op.owner );
   }

   void operator()( const vesting_balance_withdraw_operation& op ) { }
   void operator()( const worker_create_operation& op ) { }
   void operator()( const custom_operation& op ) { }
   void operator()( const assert_operation& op ) { }
   void operator()( const balance_claim_operation& op ) { }

   void operator()( const override_transfer_operation& op )
   {
      _impacted_accounts.insert( op.to );
      _impacted_accounts.insert( op.from );
      _impacted_accounts.insert( op.issuer );
   }

   void operator()( const transfer_to_blind_operation& op )
   {
      _impacted_accounts.insert( op.from );
      for( const auto& out : op.outputs )
         add_authority_accounts( _impacted_accounts, out.owner );
   }

   void operator()( const blind_transfer_operation& op )
   {
      for( const auto& in : op.inputs )
         add_authority_accounts( _impacted_accounts, in.owner );
      for( const auto& out : op.outputs )
         add_authority_accounts( _impacted_accounts, out.owner );
   }

   void operator()( const transfer_from_blind_operation& op )
   {
      _impacted_accounts.insert( op.to );

      for( const auto& in : op.inputs ) {
         add_authority_accounts(_impacted_accounts, in.owner);
      }
   }

   void operator()( const asset_settle_cancel_operation& op ) {
      _impacted_accounts.insert( op.account );
   }

   void operator()( const fba_distribute_operation& op ) {
      _impacted_accounts.insert( op.account_id );
   }

   void operator()( const cheque_create_operation& op) {
      _impacted_accounts.insert( op.account_id );
   }

   void operator()( const cheque_use_operation& op) {
      _impacted_accounts.insert( op.account_id );
   }

   void operator()( const cheque_reverse_operation& op) {
      _impacted_accounts.insert( op.account_id );
   }

   void operator()( const create_market_address_operation& op) { }
   void operator()( const account_edc_limit_daily_volume_operation& op) {
      _impacted_accounts.insert( op.account_id );
   }
   void operator()( const fund_deposit_update_operation& op )
   {
      if (db_ptr)
      {
         auto dep_itr = db_ptr->find(op.deposit_id);
         if (dep_itr)
         {
            auto fund_itr = db_ptr->find(dep_itr->fund_id);
            if (fund_itr) {
               _impacted_accounts.insert(fund_itr->owner);
            }
         }
      }
   }
   void operator()( const fund_deposit_update2_operation& op )
   {
      if (db_ptr)
      {
         auto dep_itr = db_ptr->find(op.deposit_id);
         if (dep_itr)
         {
            auto fund_itr = db_ptr->find(dep_itr->fund_id);
            if (fund_itr) {
               _impacted_accounts.insert(fund_itr->owner);
            }
         }
      }
   }
   void operator()( const fund_deposit_reduce_operation& op )
   {
      if (db_ptr)
      {
         auto dep_itr = db_ptr->find(op.deposit_id);
         if (dep_itr) {
            _impacted_accounts.insert(dep_itr->account_id);
         }
      }
   }
   void operator()( const denominate_operation& op ) {
      _impacted_accounts.insert(op.fee_payer());
   }
   void operator()( const set_witness_exception_operation& op ) {
      _impacted_accounts.insert(op.fee_payer());
   }
   void operator()( const update_referral_settings_operation& op ) { }
   void operator()( const update_accounts_referrer_operation& op )
   {
      for (const account_id_type& id: op.accounts) {
         _impacted_accounts.insert(id);
      }
   }
   void operator()( const enable_account_referral_payments_operation& op ) {
      _impacted_accounts.insert(op.account_id);
   }
};

void operation_get_impacted_items(
   const operation& op
   , fc::flat_set<account_id_type>& result_acc
   , fc::flat_set<fund_id_type>& result_fund
   , graphene::chain::database* db_ptr)
{
   get_impacted_items_visitor vtor(result_acc, result_fund, db_ptr);
   op.visit( vtor );
}

void transaction_get_impacted_items(
   const transaction& tx
   , fc::flat_set<account_id_type>& result_acc
   , fc::flat_set<fund_id_type>& result_fund
   , graphene::chain::database* db_ptr)
{
   for (const auto& op : tx.operations) {
      operation_get_impacted_items(op, result_acc, result_fund, db_ptr);
   }
}


} }