// see LICENSE.txt

#pragma once

#include <graphene/app/database_api.hpp>

#include <graphene/protocol/types.hpp>
#include <graphene/protocol/confidential.hpp>

#include <graphene/market_history/market_history_plugin.hpp>

#include <graphene/debug_witness/debug_api.hpp>

#include <graphene/net/node.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace graphene { namespace app {
   using namespace graphene::chain;
   using namespace graphene::market_history;
   using namespace fc::ecc;
   using namespace std;

   class application;

   struct listtransactions_result {
      transfer_operation   transfer;
      int               confirmations;
   };

   struct verify_range_result
   {
      bool        success;
      uint64_t    min_val;
      uint64_t    max_val;
   };
   
   struct verify_range_proof_rewind_result
   {
      bool                          success;
      uint64_t                      min_val;
      uint64_t                      max_val;
      uint64_t                      value_out;
      fc::ecc::blind_factor_type    blind_out;
      string                        message_out;
   };

   inline void reserve_op(operation_history_object& h_obj);

   /**
    * @brief The history_api class implements the RPC API for account history
    *
    * This API contains methods to access account histories
    */
   class history_api
   {
      public:
         history_api(application& app):_app(app){}

         vector<operation_history_object> get_accounts_history(unsigned limit = 100) const;

         /**
          * @brief Get operations relevant to the specificed account
          * @param account The account whose history should be queried
          * @param stop ID of the earliest operation to retrieve
          * @param limit Maximum number of operations to retrieve (must not exceed 100)
          * @param start ID of the most recent operation to retrieve
          * @return A list of operations performed by account, ordered from most recent to oldest.
          */
         vector<operation_history_object> get_account_history(account_id_type account
                                                              , operation_history_id_type stop = operation_history_id_type()
                                                              , unsigned limit = 100
                                                              , operation_history_id_type start = operation_history_id_type())const;

         vector<listtransactions_result> listtransactions(account_id_type account
                                                          , vector<string> addresses
                                                          , int count = 100) const;

         vector<operation_history_object> get_account_operation_history(
                                                            account_id_type account
                                                            , unsigned operation_type
                                                            , unsigned limit = 100) const;

         vector<operation_history_object> get_account_operation_history2(
                                                              account_id_type account
                                                              , operation_history_id_type stop = operation_history_id_type()
                                                              , unsigned limit = 100
                                                              , operation_history_id_type start = operation_history_id_type()
                                                              , unsigned operation_type = 0) const;

         vector<operation_history_object> get_account_operation_history3(
                                                              account_id_type account_id
                                                              , operation_history_id_type stop = operation_history_id_type()
                                                              , unsigned limit = 100
                                                              , operation_history_id_type start = operation_history_id_type()
                                                              , const vector<uint16_t>& operation_types = vector<uint16_t>()) const;

         // available operations: [transfer_operation(0), ...]
         vector<operation_history_object> get_account_operation_history4(
                                                              account_id_type account
                                                              , operation_history_id_type start = operation_history_id_type()
                                                              , unsigned limit = 100
                                                              , const vector<uint16_t>& operation_types = vector<uint16_t>()) const;

         vector<operation_history_object> get_account_leasing_history(account_id_type account_id
                                                              , const std::vector<fund_id_type> funds
                                                              , unsigned limit
                                                              , operation_history_id_type start) const;

         // funds
         vector<operation_history_object> get_fund_history(fund_id_type fund_id
                                                           , operation_history_id_type stop = operation_history_id_type()
                                                           , unsigned limit = 100
                                                           , operation_history_id_type start = operation_history_id_type()
                                                           , const vector<uint16_t>& operation_types = vector<uint16_t>()) const;

         // contains information about fund's payments (whole and to users)
         vector<fund_history_object::history_item> get_fund_payments_history(fund_id_type fund_id, uint32_t start, uint32_t limit) const;

         /**
          * @breif Get operations relevant to the specified account referenced
          * by an event numbering specific to the account. The current number of operations
          * for the account can be found in the account statistics (or use 0 for start).
          * @param account The account whose history should be queried
          * @param stop Sequence number of earliest operation. 0 is default and will
          * query 'limit' number of operations.
          * @param limit Maximum number of operations to retrieve (must not exceed 100)
          * @param start Sequence number of the most recent operation to retrieve.
          * 0 is default, which will start querying from the most recent operation.
          * @return A list of operations performed by account, ordered from most recent to oldest.
          */
         vector<operation_history_object> get_relative_history( account_id_type account,
                                                                        uint32_t stop = 0,
                                                                        unsigned limit = 100,
                                                                        uint32_t start = 0) const;

         vector<order_history_object> get_fill_order_history( asset_id_type a, asset_id_type b, uint32_t limit )const;
         vector<bucket_object> get_market_history( asset_id_type a, asset_id_type b, uint32_t bucket_seconds,
                                                   fc::time_point_sec start, fc::time_point_sec end )const;
         flat_set<uint32_t> get_market_history_buckets()const;

      private:
           application& _app;
   };

   /**
    * @brief The secure_api class implements the RPC API for secure elements
    *
    * This API contains methods to access to various secure entities
    */
   class secure_api
   {
   public:
      secure_api(application& app):_app(app) { }

      fc::variants get_objects(const vector<object_id_type>& ids) const;

      std::vector<blind_transfer2_object>
      get_account_blind_transfers2(account_id_type account_id, uint32_t start, uint32_t limit) const;

      std::vector<cheque_object>
      get_account_cheques(account_id_type account_id, uint32_t start, uint32_t limit) const;

   private:
      application& _app;

   };

   /**
    * @brief The network_broadcast_api class allows broadcasting of transactions.
    */
   class network_broadcast_api : public std::enable_shared_from_this<network_broadcast_api>
   {
      public:
         network_broadcast_api(application& a);

         struct transaction_confirmation
         {
            transaction_id_type   id;
            uint32_t              block_num;
            uint32_t              trx_num;
            processed_transaction trx;
         };

         typedef std::function<void(variant/*transaction_confirmation*/)> confirmation_callback;

         /**
          * @brief Broadcast a transaction to the network
          * @param trx The transaction to broadcast
          *
          * The transaction will be checked for validity in the local database prior to broadcasting. If it fails to
          * apply locally, an error will be thrown and the transaction will not be broadcast.
          */
         void broadcast_transaction(const signed_transaction& trx);

         /** this version of broadcast transaction registers a callback method that will be called when the transaction is
          * included into a block.  The callback method includes the transaction id, block number, and transaction number in the
          * block.
          */
         void broadcast_transaction_with_callback( confirmation_callback cb, const signed_transaction& trx);
         void broadcast_transaction_with_callback_new( confirmation_callback cb, const signed_transaction& trx);

         void broadcast_block( const signed_block& block );

         signed_transaction broadcast_bonus(string account_name, string amount);

         /**
          * @brief Not reflected, thus not accessible to API clients.
          *
          * This function is registered to receive the applied_block
          * signal from the chain database when a block is received.
          * It then dispatches callbacks to clients who have requested
          * to be notified when a particular txid is included in a block.
          */
         void on_applied_block( const signed_block& b );
      private:
         boost::signals2::scoped_connection             _applied_block_connection;
         map<transaction_id_type,confirmation_callback> _callbacks;
         application&                                   _app;
   };

   /**
    * @brief The network_node_api class allows maintenance of p2p connections.
    */
   class network_node_api
   {
      public:
         network_node_api(application& a);

         /**
          * @brief Return general network information, such as p2p port
          */
         fc::variant_object get_info() const;

         /**
          * @brief add_node Connect to a new peer
          * @param ep The IP/Port of the peer to connect to
          */
         void add_node(const fc::ip::endpoint& ep);

         /**
          * @brief Get status of all current connections to peers
          */
         std::vector<net::peer_status> get_connected_peers() const;

         /**
          * @brief Get advanced node parameters, such as desired and max
          *        number of connections
          */
         fc::variant_object get_advanced_node_parameters() const;

         /**
          * @brief Set advanced node parameters, such as desired and max
          *        number of connections
          * @param params a JSON object containing the name/value pairs for the parameters to set
          */
         void set_advanced_node_parameters(const fc::variant_object& params);

         /**
          * @brief Return list of potential peers
          */
         std::vector<net::potential_peer_record> get_potential_peers() const;

      private:
         application& _app;
   };
   
   class crypto_api
   {
      public:
         crypto_api();
         
         /*fc::ecc::blind_signature blind_sign( const extended_private_key_type& key, const fc::ecc::blinded_hash& hash, int i );
         
         signature_type unblind_signature( const extended_private_key_type& key,
                                              const extended_public_key_type& bob,
                                              const fc::ecc::blind_signature& sig,
                                              const fc::sha256& hash,
                                              int i );
             */
         fc::ecc::commitment_type blind( const fc::ecc::blind_factor_type& blind, uint64_t value );
         
         fc::ecc::blind_factor_type blind_sum( const std::vector<blind_factor_type>& blinds_in, uint32_t non_neg );
         
         bool verify_sum( const std::vector<commitment_type>& commits_in, const std::vector<commitment_type>& neg_commits_in, int64_t excess );
         
         verify_range_result verify_range( const fc::ecc::commitment_type& commit, const std::vector<char>& proof );
         
         std::vector<char> range_proof_sign( uint64_t min_value, 
                                             const commitment_type& commit, 
                                             const blind_factor_type& commit_blind, 
                                             const blind_factor_type& nonce,
                                             int8_t base10_exp,
                                             uint8_t min_bits,
                                             uint64_t actual_value );
                                       
         
         verify_range_proof_rewind_result verify_range_proof_rewind( const blind_factor_type& nonce,
                                                                     const fc::ecc::commitment_type& commit, 
                                                                     const std::vector<char>& proof );
         
                                         
         range_proof_info range_get_info( const std::vector<char>& proof );
   };
} } // graphene::app
   
extern template class fc::api<graphene::app::network_broadcast_api>;
extern template class fc::api<graphene::app::network_node_api>;
extern template class fc::api<graphene::app::history_api>;
extern template class fc::api<graphene::app::crypto_api>;
extern template class fc::api<graphene::app::database_api>;
extern template class fc::api<graphene::debug_witness::debug_api>;
  
namespace graphene { namespace app {
   /**
    * @brief The login_api class implements the bottom layer of the RPC API
    *
    * All other APIs must be requested from this API.
    */
   class login_api
   {
      public:
         login_api(application& a);
         ~login_api();

         /**
          * @brief Authenticate to the RPC server
          * @param user Username to login with
          * @param password Password to login with
          * @return True if logged in successfully; false otherwise
          *
          * @note This must be called prior to requesting other APIs. Other APIs may not be accessible until the client
          * has sucessfully authenticated.
          */
         bool login(const string& user, const string& password);
         /// @brief Retrieve the network broadcast API
         fc::api<network_broadcast_api> network_broadcast()const;
         /// @brief Retrieve the database API
         fc::api<database_api> database()const;
         /// @brief Retrieve the history API
         fc::api<history_api> history()const;
         /// @brief Retrieve the secure API
         fc::api<secure_api> secure()const;
         /// @brief Retrieve the network node API
         fc::api<network_node_api> network_node()const;
         /// @brief Retrieve the cryptography API
         fc::api<crypto_api> crypto()const;
         /// @brief Retrieve the debug API (if available)
         fc::api<graphene::debug_witness::debug_api> debug()const;

      private:
         /// @brief Called to enable an API, not reflected.
         void enable_api( const string& api_name );

         application& _app;
         optional<fc::api<database_api>>          _database_api;
         optional<fc::api<network_broadcast_api>> _network_broadcast_api;
         optional<fc::api<network_node_api>>      _network_node_api;
         optional<fc::api<history_api>>           _history_api;
         optional<fc::api<secure_api>>            _secure_api;
         optional<fc::api<crypto_api>>            _crypto_api;
         optional<fc::api<graphene::debug_witness::debug_api>> _debug_api;
   };

}}  // graphene::app

extern template class fc::api<graphene::app::login_api>;

FC_REFLECT( graphene::app::listtransactions_result, (transfer)(confirmations) );
FC_REFLECT( graphene::app::network_broadcast_api::transaction_confirmation,
        (id)(block_num)(trx_num)(trx) )
FC_REFLECT( graphene::app::verify_range_result,
        (success)(min_val)(max_val) )
FC_REFLECT( graphene::app::verify_range_proof_rewind_result,
        (success)(min_val)(max_val)(value_out)(blind_out)(message_out) )
//FC_REFLECT_TYPENAME( fc::ecc::compact_signature );
//FC_REFLECT_TYPENAME( fc::ecc::commitment_type );

FC_API(graphene::app::history_api,
       (get_accounts_history)
       (get_account_history)
       (listtransactions)
       (get_account_operation_history)
       (get_account_operation_history2)
       (get_account_operation_history3)
       (get_account_operation_history4)
       (get_account_leasing_history)
       (get_fund_history)
       (get_fund_payments_history)
       (get_relative_history)
       (get_fill_order_history)
       (get_market_history)
       (get_market_history_buckets)
     )
FC_API(graphene::app::secure_api,
       (get_objects)
       (get_account_blind_transfers2)
       (get_account_cheques)
)
FC_API(graphene::app::network_broadcast_api,
       (broadcast_transaction)
       (broadcast_transaction_with_callback)
       (broadcast_transaction_with_callback_new)
       (broadcast_block)
       (broadcast_bonus)
     )
FC_API(graphene::app::network_node_api,
       (get_info)
       (add_node)
       (get_connected_peers)
       (get_potential_peers)
       (get_advanced_node_parameters)
       (set_advanced_node_parameters)
     )
FC_API(graphene::app::crypto_api,
       /*(blind_sign)
       (unblind_signature)*/
       (blind)
       (blind_sum)
       (verify_sum)
       (verify_range)
       (range_proof_sign)
       (verify_range_proof_rewind)
       (range_get_info)
     )
FC_API(graphene::app::login_api,
       (login)
       (network_broadcast)
       (database)
       (history)
       (secure)
       (network_node)
       (crypto)
       (debug)
     )
